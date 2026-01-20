import pygame
import sys
import uart

# Constants for 5" screen (800x480)
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 480
FPS = 60

# Light Mode Colors
LIGHT_PALETTE = {
    'BG': (235, 240, 248),
    'TEXT': (15, 25, 45),
    'BUTTON_BORDER': (0, 0, 0),
	'SCORE': (15, 25, 45),
    'EXIT': (255, 0, 0),
    'EXIT_HOVER': (200, 0, 0),
}

# Dark Mode Colors
DARK_PALETTE = {
    'BG': (25, 28, 35),
    'TEXT': (220, 225, 235),
    'BUTTON_BORDER': (100, 110, 130),
    'LEVEL1': (0, 200, 85),
    'LEVEL1_HOVER': (0, 230, 100),
    'LEVEL2': (60, 150, 255),
    'LEVEL2_HOVER': (90, 180, 255),
    'LEVEL3': (150, 100, 255),
    'LEVEL3_HOVER': (180, 130, 255),
    'EXIT': (255, 70, 70),
    'EXIT_HOVER': (255, 100, 100),
    'HIGHSCORE': (255, 180, 70),
}

