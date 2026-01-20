# pause_screen.py
import pygame
import sys
import uart

# Constants for 5" screen (800x480)
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 480
FPS = 60

# Colors
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
GREEN = (0, 255, 0)
RED = (255, 0, 0)
BLUE = (100, 150, 255)
GRAY = (100, 100, 100)

class Button:
    def __init__(self, x, y, width, height, text, color, hover_color):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color = color
        self.hover_color = hover_color
        self.is_hovered = False
    
    def draw(self, screen, font):
        color = self.hover_color if self.is_hovered else self.color
        pygame.draw.rect(screen, color, self.rect)
        pygame.draw.rect(screen, BLACK, self.rect, 3)
        
        text_surface = font.render(self.text, True, BLACK)
        text_rect = text_surface.get_rect(center=self.rect.center)
        screen.blit(text_surface, text_rect)
    
    def check_hover(self, mouse_pos):
        self.is_hovered = self.rect.collidepoint(mouse_pos)
    
    def is_clicked(self, mouse_pos):
        return self.rect.collidepoint(mouse_pos)

def parse_uart_command(command_str):
    """Parse UART command"""
    try:
        parts = command_str.strip().upper().split()
        if len(parts) >= 1:
            return parts[0]
    except (ValueError, IndexError):
        pass
    return None

def pause_screen(screen, clock, game_surface):
    """
    Pause screen overlay that can be called from any level
    
    Args:
        screen: pygame display surface
        clock: pygame clock
        game_surface: snapshot of the game screen to show in background
    
    Returns:
        "resume" to continue playing
        "restart" to restart the level
        "menu" to return to main menu
    """
    title_font = pygame.font.Font(None, 56)
    button_font = pygame.font.Font(None, 32)
    
    # Adjusted button positions and sizes for 5" screen
    button_width = 220
    button_height = 50
    start_x = (WINDOW_WIDTH - button_width) // 2
    start_y = 140
    button_spacing = 65
    
    resume_button = Button(start_x, start_y, button_width, button_height, 
                          "Resume", GREEN, (0, 200, 0))
    restart_button = Button(start_x, start_y + button_spacing, button_width, button_height, 
                           "Restart", BLUE, (80, 120, 200))
    menu_button = Button(start_x, start_y + button_spacing * 2, button_width, button_height, 
                        "Main Menu", RED, (200, 0, 0))
    
    while True:
        mouse_pos = pygame.mouse.get_pos()
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE or event.key == pygame.K_p:
                    return "resume"
            
            if event.type == pygame.MOUSEBUTTONDOWN:
                if resume_button.is_clicked(mouse_pos):
                    return "resume"
                if restart_button.is_clicked(mouse_pos):
                    return "restart"
                if menu_button.is_clicked(mouse_pos):
                    return "menu"
        
        # Check for UART UNPAUSE command
        command_str = uart.get_command()
        if command_str:
            command = parse_uart_command(command_str)
            if command == "UNPAUSE":
                return "resume"
            elif command == "RESTART":
                return "restart"
            elif command == "MENU":
                return "menu"
        
        resume_button.check_hover(mouse_pos)
        restart_button.check_hover(mouse_pos)
        menu_button.check_hover(mouse_pos)
        
        # Draw dimmed game background
        screen.blit(game_surface, (0, 0))
        overlay = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT))
        overlay.set_alpha(180)
        overlay.fill(GRAY)
        screen.blit(overlay, (0, 0))
        
        # Draw title
        title_text = title_font.render("PAUSED", True, WHITE)
        title_rect = title_text.get_rect(center=(WINDOW_WIDTH // 2, 60))
        
        # Draw title shadow
        shadow_text = title_font.render("PAUSED", True, BLACK)
        shadow_rect = shadow_text.get_rect(center=(WINDOW_WIDTH // 2 + 2, 62))
        screen.blit(shadow_text, shadow_rect)
        screen.blit(title_text, title_rect)
        
        # Draw buttons
        resume_button.draw(screen, button_font)
        restart_button.draw(screen, button_font)
        menu_button.draw(screen, button_font)
        
        # Draw instruction
        instruction_font = pygame.font.Font(None, 20)
        instruction_text = instruction_font.render("Press ESC or P to resume | Send UNPAUSE via UART", True, WHITE)
        instruction_rect = instruction_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT - 30))
        screen.blit(instruction_text, instruction_rect)
        
        pygame.display.flip()
        clock.tick(FPS)