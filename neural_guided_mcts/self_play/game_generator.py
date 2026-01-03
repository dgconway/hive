"""
Self-Play Game Generator
Generates training data by playing games against itself.
"""
import torch
import numpy as np
from typing import List, Tuple, Dict
from dataclasses import dataclass
import sys
import os
import time
import logging 

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from neural_guided_mcts.game_interface import GameInterface, GameState, Action
from neural_guided_mcts.state_encoder import StateEncoder
from neural_guided_mcts.action_space import ActionEncoder, TOTAL_ACTIONS
from neural_guided_mcts.network.model import BugsNet
from neural_guided_mcts.mcts.search import MCTS


@dataclass
class TrainingExample:
    """Single training example from self-play."""
    state_tensor: torch.Tensor  # Encoded state
    policy_target: np.ndarray  # MCTS policy (sparse: indices and probs)
    value_target: float  # Game outcome from this player's perspective


class SelfPlayGame:
    """Generates a single self-play game."""
    
    def __init__(
        self,
        network: BugsNet,
        game_interface: GameInterface,
        state_encoder: StateEncoder,
        action_encoder: ActionEncoder,
        num_simulations: int = 100,
        temperature_threshold: int = 30,  # Use temperature=1 for first N moves
        device: str = 'cpu',
    ):
        self.network = network
        self.game_interface = game_interface
        self.state_encoder = state_encoder
        self.action_encoder = action_encoder
        self.num_simulations = num_simulations
        self.temperature_threshold = temperature_threshold
        self.device = device
        
        self.mcts = MCTS(
            network=network,
            game_interface=game_interface,
            state_encoder=state_encoder,
            action_encoder=action_encoder,
            num_simulations=num_simulations,
            device=device,
        )
    
    def play_game(self, max_moves: int = 400) -> List[TrainingExample]:
        """
        Play a complete game and return training examples.
        
        Returns:
            List of TrainingExample from this game
        """
        examples = []
        
        state = self.game_interface.get_initial_state()
        move_count = 0
        
        while not state.is_terminal and move_count < max_moves:
            # Temperature scheduling
            temperature = 1.0 if move_count < self.temperature_threshold else 0.1
            
            # Run MCTS
            action, action_probs = self.mcts.get_action_with_search(state, temperature)
            
            if action is None:
                break
            
            # Store training example (will update value later)
            state_tensor = self.state_encoder.get_canonical_form(
                state.game, state.current_player
            )
            
            # Convert action_probs to dense policy vector
            policy_target = np.zeros(TOTAL_ACTIONS, dtype=np.float32)
            for idx, prob in action_probs.items():
                policy_target[idx] = prob
            
            examples.append({
                'state': state_tensor,
                'policy': policy_target,
                'player': state.current_player,
            })
            
            # Apply action
            try:
                state = self.game_interface.apply_action(state, action)
            except Exception as e:
                print(f"Error applying action: {e}")
                break
            
            move_count += 1
        
        # Game ended - assign values
        training_examples = []
        for ex in examples:
            value = state.get_reward(ex['player'])
            training_examples.append(TrainingExample(
                state_tensor=ex['state'],
                policy_target=ex['policy'],
                value_target=value,
            ))
        
        return training_examples


class ExperienceBuffer:
    """Circular buffer for training examples."""
    
    def __init__(self, capacity: int = 100000):
        self.capacity = capacity
        self.buffer: List[TrainingExample] = []
        self.position = 0
    
    def push(self, example: TrainingExample):
        """Add example to buffer."""
        if len(self.buffer) < self.capacity:
            self.buffer.append(example)
        else:
            self.buffer[self.position] = example
        self.position = (self.position + 1) % self.capacity
    
    def push_batch(self, examples: List[TrainingExample]):
        """Add multiple examples."""
        for ex in examples:
            self.push(ex)
    
    def sample(self, batch_size: int) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        """
        Sample a batch of examples.
        
        Returns:
            (states, policy_targets, value_targets)
        """
        indices = np.random.choice(len(self.buffer), size=batch_size, replace=False)
        
        states = torch.stack([self.buffer[i].state_tensor for i in indices])
        policies = torch.from_numpy(np.stack([self.buffer[i].policy_target for i in indices]))
        values = torch.tensor([self.buffer[i].value_target for i in indices], dtype=torch.float32)
        
        return states, policies, values.unsqueeze(1)
    
    def __len__(self):
        return len(self.buffer)


def generate_self_play_games(
    network: BugsNet,
    num_games: int,
    num_simulations: int = 100,
    device: str = 'cpu',
    verbose: bool = True,
) -> List[TrainingExample]:
    """
    Generate multiple self-play games.
    
    Returns:
        List of all training examples from games
    """
    game_interface = GameInterface()
    state_encoder = StateEncoder()
    action_encoder = ActionEncoder()
    
    all_examples = []
    
    for game_num in range(num_games):
        start_time = time.time()
        
        self_play = SelfPlayGame(
            network=network,
            game_interface=game_interface,
            state_encoder=state_encoder,
            action_encoder=action_encoder,
            num_simulations=num_simulations,
            device=device,
        )
        
        examples = self_play.play_game()
        all_examples.extend(examples)
        
        elapsed = time.time() - start_time
        
        if verbose:
            print(f"Game {game_num + 1}/{num_games}: {len(examples)} positions, {elapsed:.1f}s")
    
    return all_examples
