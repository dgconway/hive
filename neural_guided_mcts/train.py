"""
Main Training Pipeline
Orchestrates self-play and training loop.
"""
import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from neural_guided_mcts.network.model import create_model
from neural_guided_mcts.network.training import Trainer, save_checkpoint, load_checkpoint
from neural_guided_mcts.self_play.game_generator import (
    generate_self_play_games, 
    ExperienceBuffer
)


def train(
    num_iterations: int = 10,
    games_per_iteration: int = 50,
    num_simulations: int = 100,
    batch_size: int = 256,
    num_training_batches: int = 100,
    checkpoint_dir: str = "checkpoints",
    device: str = 'cpu',
):
    """
    Main training loop.
    
    Each iteration:
    1. Generate self-play games
    2. Add positions to experience buffer
    3. Train network on sampled positions
    4. Save checkpoint
    """
    print("=" * 50)
    print("Neural-Guided MCTS Training")
    print("=" * 50)
    print(f"Device: {device}")
    print(f"Iterations: {num_iterations}")
    print(f"Games per iteration: {games_per_iteration}")
    print(f"MCTS simulations: {num_simulations}")
    print(f"Batch size: {batch_size}")
    print("=" * 50)
    
    # Create checkpoint directory
    os.makedirs(checkpoint_dir, exist_ok=True)
    
    # Initialize
    network = create_model(device)
    trainer = Trainer(network, device=device)
    buffer = ExperienceBuffer(capacity=100000)
    
    # Try to load existing checkpoint
    checkpoint_path = os.path.join(checkpoint_dir, "latest.pt")
    start_iteration = 0
    if os.path.exists(checkpoint_path):
        start_iteration = load_checkpoint(network, trainer.optimizer, checkpoint_path)
    
    print(f"\nModel parameters: {network.get_param_count():,}")
    print()
    
    total_games = 0
    total_positions = 0
    
    for iteration in range(start_iteration, num_iterations):
        print(f"\n{'='*50}")
        print(f"Iteration {iteration + 1}/{num_iterations}")
        print(f"{'='*50}")
        
        # 1. Self-play
        print("\n[1] Generating self-play games...")
        start_time = time.time()
        
        examples = generate_self_play_games(
            network=network,
            num_games=games_per_iteration,
            num_simulations=num_simulations,
            device=device,
            verbose=True,
        )
        
        self_play_time = time.time() - start_time
        total_games += games_per_iteration
        total_positions += len(examples)
        
        print(f"\nGenerated {len(examples)} positions in {self_play_time:.1f}s")
        print(f"Total: {total_games} games, {total_positions} positions")
        
        # 2. Add to buffer
        buffer.push_batch(examples)
        print(f"Buffer size: {len(buffer)}")
        
        # 3. Training
        if len(buffer) >= batch_size:
            print("\n[2] Training network...")
            start_time = time.time()
            
            total_loss, policy_loss, value_loss = trainer.train_epoch(
                buffer,
                batch_size=batch_size,
                num_batches=num_training_batches,
            )
            
            train_time = time.time() - start_time
            
            print(f"Training completed in {train_time:.1f}s")
            print(f"Losses - Total: {total_loss:.4f}, Policy: {policy_loss:.4f}, Value: {value_loss:.4f}")
        else:
            print("\n[2] Skipping training (not enough data)")
        
        # 4. Save checkpoint
        print("\n[3] Saving checkpoint...")
        save_checkpoint(
            network, 
            trainer.optimizer, 
            iteration + 1,
            checkpoint_path
        )
        
        # Also save numbered checkpoint every 5 iterations
        if (iteration + 1) % 5 == 0:
            numbered_path = os.path.join(checkpoint_dir, f"checkpoint_{iteration + 1}.pt")
            save_checkpoint(network, trainer.optimizer, iteration + 1, numbered_path)
    
    print("\n" + "=" * 50)
    print("Training complete!")
    print(f"Total games played: {total_games}")
    print(f"Total positions: {total_positions}")
    print("=" * 50)


def main():
    parser = argparse.ArgumentParser(description="Train BUGS AI with Neural MCTS")
    parser.add_argument("--iterations", type=int, default=8, help="Number of training iterations")
    parser.add_argument("--games", type=int, default=50, help="Games per iteration")
    parser.add_argument("--simulations", type=int, default=100, help="MCTS simulations per move")
    parser.add_argument("--batch-size", type=int, default=256, help="Training batch size")
    parser.add_argument("--checkpoint-dir", type=str, default="checkpoints", help="Checkpoint directory")
    
    args = parser.parse_args()
    
    device = 'cpu'
    
    train(
        num_iterations=args.iterations,
        games_per_iteration=args.games,
        num_simulations=args.simulations,
        batch_size=args.batch_size,
        checkpoint_dir=args.checkpoint_dir,
        device=device,
    )


if __name__ == "__main__":
    main()
