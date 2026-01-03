"""
MCTS Search Algorithm
Neural-guided Monte Carlo Tree Search implementation.
"""
import numpy as np
import torch
from typing import List, Dict, Tuple, Optional
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from neural_guided_mcts.mcts.node import MCTSNode
from neural_guided_mcts.game_interface import GameInterface, GameState, Action
from neural_guided_mcts.state_encoder import StateEncoder
from neural_guided_mcts.action_space import ActionEncoder
from neural_guided_mcts.network.model import BugsNet


class MCTS:
    """
    Monte Carlo Tree Search with neural network guidance.
    """
    
    def __init__(
        self,
        network: BugsNet,
        game_interface: GameInterface,
        state_encoder: StateEncoder,
        action_encoder: ActionEncoder,
        num_simulations: int = 100,  # Reduced for CPU
        c_puct: float = 1.5,
        device: str = 'cpu',
    ):
        self.network = network
        self.game_interface = game_interface
        self.state_encoder = state_encoder
        self.action_encoder = action_encoder
        self.num_simulations = num_simulations
        self.c_puct = c_puct
        self.device = device
    
    def search(self, state: GameState) -> Dict[int, float]:
        """
        Run MCTS from given state and return action probabilities.
        
        Args:
            state: Current game state
            
        Returns:
            Dict mapping action_idx -> visit probability
        """
        root = MCTSNode()
        
        # Expand root with neural network evaluation
        self._expand_node(root, state)
        
        # Run simulations
        for _ in range(self.num_simulations):
            self._simulate(root, state)
        
        return root.get_action_probs(temperature=1.0)
    
    def get_action_with_search(
        self, 
        state: GameState, 
        temperature: float = 1.0
    ) -> Tuple[Action, Dict[int, float]]:
        """
        Run MCTS and select an action.
        
        Args:
            state: Current game state
            temperature: Temperature for action selection
            
        Returns:
            (selected_action, policy_dict)
        """
        root = MCTSNode()
        
        # Expand root
        self._expand_node(root, state)
        
        # Run simulations
        for _ in range(self.num_simulations):
            self._simulate(root, state)
        
        # Get action probabilities
        action_probs = root.get_action_probs(temperature=temperature)
        
        if not action_probs:
            # No valid actions (should not happen in non-terminal states)
            return None, {}
        
        # Sample action according to probabilities
        if temperature == 0:
            # Deterministic
            action_idx = max(action_probs, key=action_probs.get)
        else:
            # Sample
            actions = list(action_probs.keys())
            probs = [action_probs[a] for a in actions]
            # necessary to precisely normalize
            action_idx = np.random.choice(actions, p=(probs / sum(probs)))
        
        # Decode action
        action_dict = self.action_encoder.decode(action_idx)
        action = Action(
            action_type=action_dict["action_type"],
            piece_type=action_dict["piece_type"],
            from_hex=action_dict["from_hex"],
            to_hex=action_dict["to_hex"]
        )
        
        return action, action_probs
    
    def _simulate(self, root: MCTSNode, root_state: GameState):
        """Run a single MCTS simulation."""
        node = root
        state = root_state.copy()
        path = [node]
        
        # Selection: traverse tree until leaf
        while node.is_expanded and not state.is_terminal:
            action_idx, node = node.select_child(self.c_puct)
            path.append(node)
            
            # Apply action
            action_dict = self.action_encoder.decode(action_idx)
            action = Action(
                action_type=action_dict["action_type"],
                piece_type=action_dict["piece_type"],
                from_hex=action_dict["from_hex"],
                to_hex=action_dict["to_hex"]
            )
            try:
                state = self.game_interface.apply_action(state, action)
            except Exception:
                # Invalid action, assign low value
                node.backup(-1.0)
                return
        
        # Expansion and evaluation
        if state.is_terminal:
            # Game ended, use actual result
            # Reward is from root player's perspective, but we need it from leaf player's
            value = state.get_reward(state.current_player)
        else:
            # Expand with neural network
            # Returns value from state.current_player perspective
            value = self._expand_node(node, state)
        
        # Backpropagation
        node.backup(value)
    
    def _expand_node(self, node: MCTSNode, state: GameState) -> float:
        """
        Expand node using neural network.
        
        Returns:
            value: Value estimate for this state
        """
        # Encode state
        state_tensor = self.state_encoder.get_canonical_form(
            state.game, state.current_player
        ).unsqueeze(0).to(self.device)
        
        # Get neural network predictions
        self.network.eval()
        with torch.no_grad():
            log_policy, value = self.network(state_tensor)
            policy = torch.exp(log_policy).squeeze(0).cpu().numpy()
            value = value.squeeze().item()
        
        # Get legal actions
        legal_actions = self.game_interface.get_legal_actions(state)
        legal_indices = self.action_encoder.get_legal_action_mask(legal_actions)
        
        if not legal_indices:
            return value
        
        # Mask and renormalize policy
        masked_policy = {idx: policy[idx] for idx in legal_indices}
        policy_sum = sum(masked_policy.values())
        
        if policy_sum > 0:
            masked_policy = {idx: p / policy_sum for idx, p in masked_policy.items()}
        else:
            # Uniform if all zeros
            uniform_prob = 1.0 / len(legal_indices)
            masked_policy = {idx: uniform_prob for idx in legal_indices}
        
        # Expand node
        node.expand(masked_policy)
        
        return value
