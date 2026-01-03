"""
MCTS Node and Tree Structure
"""
import math
from typing import Dict, List, Optional, Tuple
import numpy as np


class MCTSNode:
    """
    Node in the MCTS tree.
    
    Stores:
    - State information
    - Statistics: visit count, value sum
    - Prior probability from neural network
    - Children nodes
    """
    
    def __init__(
        self,
        prior: float = 0.0,
        parent: Optional['MCTSNode'] = None,
        action_idx: Optional[int] = None,
    ):
        self.prior = prior  # P(s,a) from neural network
        self.parent = parent
        self.action_idx = action_idx  # Action that led to this node
        
        self.children: Dict[int, 'MCTSNode'] = {}
        
        self.visit_count = 0
        self.value_sum = 0.0
    
    @property
    def is_expanded(self) -> bool:
        """Check if node has been expanded."""
        return len(self.children) > 0
    
    @property
    def q_value(self) -> float:
        """Mean action value Q(s,a)."""
        if self.visit_count == 0:
            return 0.0
        return self.value_sum / self.visit_count
    
    def ucb_score(self, c_puct: float = 1.5) -> float:
        """
        Calculate UCB score for node selection.
        
        UCB = Q(s,a) + c_puct * P(s,a) * sqrt(N_parent) / (1 + N(s,a))
        """
        if self.parent is None:
            return 0.0
        
        exploration = c_puct * self.prior * math.sqrt(self.parent.visit_count) / (1 + self.visit_count)
        return self.q_value + exploration
    
    def select_child(self, c_puct: float = 1.5) -> Tuple[int, 'MCTSNode']:
        """Select child with highest UCB score."""
        best_score = -float('inf')
        best_action = None
        best_child = None
        
        for action_idx, child in self.children.items():
            score = child.ucb_score(c_puct)
            if score > best_score:
                best_score = score
                best_action = action_idx
                best_child = child
        
        return best_action, best_child
    
    def expand(self, action_priors: Dict[int, float]):
        """
        Expand node with children for legal actions.
        
        Args:
            action_priors: Dict mapping action_idx -> prior probability
        """
        for action_idx, prior in action_priors.items():
            if action_idx not in self.children:
                self.children[action_idx] = MCTSNode(
                    prior=prior,
                    parent=self,
                    action_idx=action_idx
                )
    
    def backup(self, value: float):
        """
        Backpropagate value up the tree.
        Value is from the perspective of the current player at this node.
        """
        node = self
        # Alternate sign because value is relative to player who made the move
        sign = 1
        while node is not None:
            node.visit_count += 1
            node.value_sum += sign * value
            sign = -sign
            node = node.parent
    
    def get_action_probs(self, temperature: float = 1.0) -> Dict[int, float]:
        """
        Get action probabilities based on visit counts.
        
        Args:
            temperature: Controls exploration vs exploitation.
                        1.0 = proportional to visits
                        0.0 = deterministic (most visited)
        """
        if not self.children:
            return {}
        
        action_visits = {
            action_idx: child.visit_count 
            for action_idx, child in self.children.items()
        }
        
        if temperature == 0:
            # Deterministic: choose action with max visits
            best_action = max(action_visits, key=action_visits.get)
            probs = {action: 0.0 for action in action_visits}
            probs[best_action] = 1.0
            return probs
        
        # Temperature-scaled probabilities
        visits = np.array(list(action_visits.values()), dtype=np.float32)
        
        if temperature == 1.0:
            # Standard: proportional to visits
            visit_sum = visits.sum()
            if visit_sum == 0:
                # Uniform if no visits
                probs = {action: 1.0 / len(action_visits) for action in action_visits}
            else:
                probs = {
                    action: count / visit_sum 
                    for action, count in action_visits.items()
                }
        else:
            # Generalized temperature
            visits_temp = np.power(visits, 1.0 / temperature)
            visit_sum = visits_temp.sum()
            probs = {
                action: visits_temp[i] / visit_sum 
                for i, action in enumerate(action_visits.keys())
            }
        
        return probs
