"""
Training Loop for Neural Network
Implements training from self-play data.
"""
import torch
import torch.nn as nn
import torch.optim as optim
from typing import Tuple, Optional
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from neural_guided_mcts.network.model import BugsNet, create_model
from neural_guided_mcts.self_play.game_generator import ExperienceBuffer


class Trainer:
    """Handles neural network training."""
    
    def __init__(
        self,
        network: BugsNet,
        learning_rate: float = 0.001,
        weight_decay: float = 1e-4,
        device: str = 'cpu',
    ):
        self.network = network
        self.device = device
        
        self.optimizer = optim.Adam(
            network.parameters(),
            lr=learning_rate,
            weight_decay=weight_decay,
        )
        
        self.scheduler = optim.lr_scheduler.StepLR(
            self.optimizer, 
            step_size=100, 
            gamma=0.9
        )
    
    def train_batch(
        self, 
        states: torch.Tensor, 
        policy_targets: torch.Tensor, 
        value_targets: torch.Tensor
    ) -> Tuple[float, float, float]:
        """
        Train on a single batch.
        
        Returns:
            (total_loss, policy_loss, value_loss)
        """
        self.network.train()
        
        states = states.to(self.device)
        policy_targets = policy_targets.to(self.device)
        value_targets = value_targets.to(self.device)
        
        # Forward pass
        log_policy, value = self.network(states)
        
        # Policy loss (cross-entropy with soft targets)
        policy_loss = -torch.sum(policy_targets * log_policy) / states.size(0)
        
        # Value loss (MSE)
        value_loss = nn.functional.mse_loss(value, value_targets)
        
        # Total loss
        total_loss = policy_loss + value_loss
        
        # Backward pass
        self.optimizer.zero_grad()
        total_loss.backward()
        
        # Gradient clipping
        torch.nn.utils.clip_grad_norm_(self.network.parameters(), max_norm=1.0)
        
        self.optimizer.step()
        
        return total_loss.item(), policy_loss.item(), value_loss.item()
    
    def train_epoch(
        self, 
        buffer: ExperienceBuffer, 
        batch_size: int = 256,
        num_batches: int = 100,
    ) -> Tuple[float, float, float]:
        """
        Train for one epoch.
        
        Returns:
            Average (total_loss, policy_loss, value_loss)
        """
        if len(buffer) < batch_size:
            print(f"Not enough data in buffer ({len(buffer)} < {batch_size})")
            return 0.0, 0.0, 0.0
        
        total_losses = []
        policy_losses = []
        value_losses = []
        
        for _ in range(num_batches):
            states, policies, values = buffer.sample(batch_size)
            total, policy, value = self.train_batch(states, policies, values)
            
            total_losses.append(total)
            policy_losses.append(policy)
            value_losses.append(value)
        
        self.scheduler.step()
        
        return (
            sum(total_losses) / len(total_losses),
            sum(policy_losses) / len(policy_losses),
            sum(value_losses) / len(value_losses),
        )


def save_checkpoint(
    network: BugsNet, 
    optimizer: optim.Optimizer,
    epoch: int,
    path: str
):
    """Save model checkpoint."""
    torch.save({
        'epoch': epoch,
        'model_state_dict': network.state_dict(),
        'optimizer_state_dict': optimizer.state_dict(),
    }, path)
    print(f"Saved checkpoint to {path}")


def load_checkpoint(
    network: BugsNet,
    optimizer: Optional[optim.Optimizer] = None,
    path: str = None,
) -> int:
    """Load model checkpoint. Returns epoch number."""
    if not os.path.exists(path):
        print(f"No checkpoint found at {path}")
        return 0
    
    checkpoint = torch.load(path)
    network.load_state_dict(checkpoint['model_state_dict'])
    
    if optimizer is not None:
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
    
    epoch = checkpoint.get('epoch', 0)
    print(f"Loaded checkpoint from {path} (epoch {epoch})")
    return epoch
