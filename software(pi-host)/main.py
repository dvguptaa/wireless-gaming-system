import pygame
import sys
import uart
from level1 import game_loop as level1_game_loop
from level2 import game_loop as level2_game_loop
from level3 import game_loop as level3_game_loop
NO_SCORE = 255

# Initialize Pygame
pygame.init()

# Constants for 5" screen (800x480)
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 480
FPS = 60
high_score_3 = NO_SCORE


def format_high_score(score):
        if score == NO_SCORE: 
            return "No Score"
        return f"{score}s"



# Light Mode Colors
LIGHT_PALETTE = {
    'BG': (235, 240, 248),
    'TEXT': (15, 25, 45),
    'BUTTON_BORDER': (0, 0, 0),
    'LEVEL1': (0, 255, 0),
    'LEVEL1_HOVER': (0, 200, 0),
    'LEVEL2': (100, 150, 255),
    'LEVEL2_HOVER': (80, 120, 200),
    'LEVEL3': (150, 100, 255),
    'LEVEL3_HOVER': (120, 80, 200),
    'EXIT': (255, 0, 0),
    'EXIT_HOVER': (200, 0, 0),
    'HIGHSCORE': (220, 150, 50),
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

# Global theme state
current_theme = 'light'
colors = LIGHT_PALETTE.copy()

def set_theme(is_dark):
    """Set theme directly: True for dark, False for light"""
    global current_theme, colors
    if is_dark and current_theme == 'light':
        current_theme = 'dark'
        colors = DARK_PALETTE.copy()
    elif not is_dark and current_theme == 'dark':
        current_theme = 'light'
        colors = LIGHT_PALETTE.copy()

def parse_uart_command(command_str):
    """Parse UART command like 'DARK 1' for theme control or 'S 123' for high score"""
    try:
        parts = command_str.strip().upper().split()
        if len(parts) == 2:
            command = parts[0]
            value = int(parts[1])
            return command, value
        elif len(parts) == 1:
            return parts[0], 1
    except (ValueError, IndexError):
        pass
    return None, 0

class Button:
    def __init__(self, x, y, width, height, text, color_key, hover_color_key):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color_key = color_key
        self.hover_color_key = hover_color_key
        self.is_hovered = False
    def draw(self, screen, font):
        color = colors[self.hover_color_key] if self.is_hovered else colors[self.color_key]
        pygame.draw.rect(screen, color, self.rect, border_radius=8)
        pygame.draw.rect(screen, colors['BUTTON_BORDER'], self.rect, 3, border_radius=8)
        
        text_surface = font.render(self.text, True, colors['TEXT'])
        text_rect = text_surface.get_rect(center=self.rect.center)
        screen.blit(text_surface, text_rect)
    
    def check_hover(self, mouse_pos):
        self.is_hovered = self.rect.collidepoint(mouse_pos)
    
    def is_clicked(self, mouse_pos):
        return self.rect.collidepoint(mouse_pos)

def main_menu(screen, clock, high_score):
    # Start UART polling
    uart.start_polling()

    uart.send_event("MENU")
    
    title_font = pygame.font.Font(None, 56)
    button_font = pygame.font.Font(None, 32)
    highscore_font = pygame.font.Font(None, 42)

    # Adjusted button positions and sizes for 5" screen
    button_width = 220
    button_height = 50
    start_x = (WINDOW_WIDTH - button_width) // 2
    start_y = 120
    button_spacing = 65
    
    level1_button = Button(start_x, start_y, button_width, button_height, 
                          "Level 1", 'LEVEL1', 'LEVEL1_HOVER')
    level2_button = Button(start_x, start_y + button_spacing, button_width, button_height, 
                          "Level 2", 'LEVEL2', 'LEVEL2_HOVER')
    level3_button = Button(start_x, start_y + button_spacing * 2, button_width, button_height, 
                          "Level 3", 'LEVEL3', 'LEVEL3_HOVER')
    exit_button = Button(start_x, start_y + button_spacing * 3, button_width, button_height, 
                        "Exit", 'EXIT', 'EXIT_HOVER')
    
    
    
    try:
        while True:
            mouse_pos = pygame.mouse.get_pos()
            
            # Process UART commands for theme control and high score
            command_str = uart.get_command()
            if command_str:
                command, value = parse_uart_command(command_str)
                if command == "DARK":
                    # Handle theme toggle: DARK 1 = dark mode, DARK 0 = light mode
                    set_theme(value == 1)
                elif command == "S":
                    # Handle high score: S <number>
                    high_score = value
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                
                if event.type == pygame.MOUSEBUTTONDOWN:
                    # Check level buttons
                    if level1_button.is_clicked(mouse_pos):
                        uart.stop_polling()
                        return "level1"
                    elif level2_button.is_clicked(mouse_pos):
                        uart.stop_polling()
                        return "level2"
                    elif level3_button.is_clicked(mouse_pos):
                        uart.stop_polling()
                        return "level3"
                    elif exit_button.is_clicked(mouse_pos):
                        uart.stop_polling()
                        pygame.quit()
                        sys.exit()
            
            level1_button.check_hover(mouse_pos)
            level2_button.check_hover(mouse_pos)
            level3_button.check_hover(mouse_pos)
            exit_button.check_hover(mouse_pos)
            
            screen.fill(colors['BG'])
            
            # Draw title
            title_text = title_font.render("MAZE GAME", True, colors['TEXT'])
            title_rect = title_text.get_rect(center=(WINDOW_WIDTH // 2, 50))
            screen.blit(title_text, title_rect)
            
            # Draw buttons
            level1_button.draw(screen, button_font)
            level2_button.draw(screen, button_font)
            level3_button.draw(screen, button_font)
            exit_button.draw(screen, button_font)



            high_score_text = highscore_font.render(f"Best Time: {format_high_score(high_score_3)}",
                                                    True,
                                                    colors['HIGHSCORE'])

            
            # Draw high score if available
            if high_score is not None:
                if high_score == NO_SCORE:
                    highscore_text = highscore_font.render(f"No Score Set", True, colors['HIGHSCORE'])
                    highscore_rect = highscore_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT - 80))
                    screen.blit(highscore_text, highscore_rect)
                else:
                    highscore_text = highscore_font.render(f"BEST TIME: {high_score} seconds", True, colors['HIGHSCORE'])
                    highscore_rect = highscore_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT - 80))
                    screen.blit(highscore_text, highscore_rect)
            
            pygame.display.flip()
            clock.tick(FPS)
    
    finally:
        uart.stop_polling()

def main():
    # Optional: Set display mode for Raspberry Pi touchscreen
    # Uncomment the following line if you want fullscreen on Pi
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.FULLSCREEN)
    pygame.mouse.set_visible(False)
    
    # screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Maze Game")
    clock = pygame.time.Clock()
    
    current_state = "menu"
    
    while True:
        if current_state == "menu":
            current_state = main_menu(screen, clock, high_score_3)
        elif current_state == "level1":
            uart.send_event("LEVEL")
            current_state = level1_game_loop(screen, clock, 0)
        elif current_state == "level2":
            uart.send_event("LEVEL")
            current_state = level2_game_loop(screen, clock, 0)
        elif current_state == "level3":
            uart.send_event("LEVEL")
            current_state = level3_game_loop(screen, clock, high_score_3)

if __name__ == "__main__":
    main()
