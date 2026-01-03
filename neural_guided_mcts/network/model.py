"""
Neural Network Architecture for BUGS Game AI
ResNet-style network with policy and value heads.
Optimized for CPU training.
"""
import torch
import torch.nn as nn
import torch.nn.functional as F
from typing import Tuple
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from neural_guided_mcts.state_encoder import NUM_CHANNELS, GRID_SIZE
from neural_guided_mcts.action_space import TOTAL_ACTIONS


class ResidualBlock(nn.Module):
    """Residual block with two conv layers and skip connection."""
    
    def __init__(self, num_filters: int):
        super().__init__()
        self.conv1 = nn.Conv2d(num_filters, num_filters, kernel_size=3, padding=1)
        self.bn1 = nn.BatchNorm2d(num_filters)
        self.conv2 = nn.Conv2d(num_filters, num_filters, kernel_size=3, padding=1)
        self.bn2 = nn.BatchNorm2d(num_filters)
    
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        residual = x
        x = F.relu(self.bn1(self.conv1(x)))
        x = self.bn2(self.conv2(x))
        x = F.relu(x + residual)
        return x


class BugsNet(nn.Module):
    """
    Neural network for BUGS game.
    
    Architecture:
    - Input: (batch, NUM_CHANNELS, GRID_SIZE, GRID_SIZE)
    - Initial conv layer
    - N residual blocks
    - Policy head: outputs action probabilities
    - Value head: outputs single value in [-1, 1]
    """
    
    def __init__(
        self,
        num_filters: int = 64,  # Reduced for CPU
        num_res_blocks: int = 4,  # Reduced for CPU
        policy_filters: int = 2,
        value_filters: int = 2,
    ):
        super().__init__()
        
        self.num_filters = num_filters
        self.num_res_blocks = num_res_blocks
        
        # Initial convolution
        self.initial_conv = nn.Conv2d(NUM_CHANNELS, num_filters, kernel_size=3, padding=1)
        self.initial_bn = nn.BatchNorm2d(num_filters)
        
        # Residual blocks
        self.res_blocks = nn.ModuleList([
            ResidualBlock(num_filters) for _ in range(num_res_blocks)
        ])
        
        # Policy head
        self.policy_conv = nn.Conv2d(num_filters, policy_filters, kernel_size=1)
        self.policy_bn = nn.BatchNorm2d(policy_filters)
        self.policy_fc = nn.Linear(policy_filters * GRID_SIZE * GRID_SIZE, TOTAL_ACTIONS)
        
        # Value head
        self.value_conv = nn.Conv2d(num_filters, value_filters, kernel_size=1)
        self.value_bn = nn.BatchNorm2d(value_filters)
        self.value_fc1 = nn.Linear(value_filters * GRID_SIZE * GRID_SIZE, 256)
        self.value_fc2 = nn.Linear(256, 1)
    
    def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Forward pass.
        
        Args:
            x: Input tensor of shape (batch, NUM_CHANNELS, GRID_SIZE, GRID_SIZE)
            
        Returns:
            policy: Log probabilities over actions, shape (batch, TOTAL_ACTIONS)
            value: Value estimate in [-1, 1], shape (batch, 1)
        """
        # Initial conv
        x = F.relu(self.initial_bn(self.initial_conv(x)))
        
        # Residual blocks
        for block in self.res_blocks:
            x = block(x)
        
        # Policy head
        policy = F.relu(self.policy_bn(self.policy_conv(x)))
        policy = policy.view(policy.size(0), -1)
        policy = self.policy_fc(policy)
        policy = F.log_softmax(policy, dim=1)
        
        # Value head
        value = F.relu(self.value_bn(self.value_conv(x)))
        value = value.view(value.size(0), -1)
        value = F.relu(self.value_fc1(value))
        value = torch.tanh(self.value_fc2(value))
        
        return policy, value
    
    def predict(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Get policy probabilities and value (for inference).
        
        Returns:
            policy_probs: Probabilities (not log), shape (batch, TOTAL_ACTIONS)
            value: Value estimate, shape (batch, 1)
        """
        self.eval()
        with torch.no_grad():
            log_policy, value = self.forward(x)
            policy_probs = torch.exp(log_policy)
        return policy_probs, value
    
    def get_param_count(self) -> int:
        """Return total number of trainable parameters."""
        return sum(p.numel() for p in self.parameters() if p.requires_grad)


def create_model(device: str = 'cpu') -> BugsNet:
    """Create and return model on specified device."""
    model = BugsNet()
    model = model.to(device)
    return model
