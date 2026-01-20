# level1.py - Complete game with win gesture detection
import pygame
import sys
import math
import uart
from pause_screen import pause_screen

# Constants for 5" screen (800x480)
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 480
CELL_SIZE = 26
FPS = 120
MOVE_DELAY = 35
BOUNCE_COOLDOWN = 300
RUMBLE_COOLDOWN = 2000  # Minimum time between rumble signals (ms)
SMOOTH_MOVE_DURATION = 12

# UI element heights
TOP_UI_HEIGHT = 3
BOTTOM_UI_HEIGHT = 30

# Light Mode Color Palette
LIGHT_PALETTE = {
    'WALL_BASE': (12, 15, 28),
    'WALL_FACE': (28, 35, 55),
    'WALL_LIGHT': (48, 58, 82),
    'WALL_BRIGHT': (68, 82, 110),
    'WALL_SHADOW': (5, 8, 15),
    'PATH_BASE': (215, 222, 232),
    'PATH_ALT': (235, 240, 248),
    'PLAYER_CORE': (40, 130, 245),
    'PLAYER_MID': (70, 160, 255),
    'PLAYER_LIGHT': (140, 200, 255),
    'PLAYER_SHINE': (220, 240, 255),
    'GOAL_CORE': (0, 200, 85),
    'GOAL_GLOW': (80, 255, 140),
    'GOAL_BRIGHT': (180, 255, 200),
    'TEXT_COLOR': (15, 25, 45),
    'GRID_COLOR': (195, 205, 215),
    'SHADOW_ALPHA': 70,
}

# Dark Mode Color Palette
DARK_PALETTE = {
    'WALL_BASE': (45, 50, 65),
    'WALL_FACE': (55, 62, 82),
    'WALL_LIGHT': (70, 78, 100),
    'WALL_BRIGHT': (85, 95, 120),
    'WALL_SHADOW': (20, 22, 30),
    'PATH_BASE': (25, 28, 35),
    'PATH_ALT': (30, 33, 42),
    'PLAYER_CORE': (60, 150, 255),
    'PLAYER_MID': (90, 180, 255),
    'PLAYER_LIGHT': (150, 210, 255),
    'PLAYER_SHINE': (230, 245, 255),
    'GOAL_CORE': (20, 220, 100),
    'GOAL_GLOW': (100, 255, 160),
    'GOAL_BRIGHT': (200, 255, 220),
    'TEXT_COLOR': (220, 225, 235),
    'GRID_COLOR': (50, 55, 65),
    'SHADOW_ALPHA': 90,
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

# Maze layout (1 = wall, 0 = path, 2 = goal)
maze = [
    [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    [1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
    [1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1],
    [1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1],
    [1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1],
    [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
    [1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1],
    [1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
    [1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1],
    [1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1],
    [1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1],
    [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1],
    [1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1],
    [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1],
    [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
]

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.display_x = float(x)
        self.display_y = float(y)
        self.start_x = x
        self.start_y = y
        self.target_x = x
        self.target_y = y
        self.move_progress = 0
        self.trail = []
        self.trail_max = 20
        self.bounce_offset_x = 0
        self.bounce_offset_y = 0
        self.bounce_duration = 0
        self.last_bounce_time = 0
        self.last_rumble_time = 0
        self.move_queue = []
        self.path = []
        self.total_distance = 0
        self.is_moving = False
        self.will_hit_wall = False
        self.wall_direction = None
        self.ignore_commands = 0
    
    def queue_moves(self, dx, dy, count):
        """Queue multiple moves and calculate the complete path"""
        temp_x = self.x
        temp_y = self.y
        new_path = [(temp_x, temp_y)]
        hit_wall = False
        
        for i in range(count):
            new_x = temp_x + dx
            new_y = temp_y + dy
            
            if maze[new_y][new_x] != 1:
                temp_x = new_x
                temp_y = new_y
                new_path.append((temp_x, temp_y))
            else:
                hit_wall = True
                break
        
        if len(new_path) > 1:
            self.path = new_path
            self.start_x = self.x
            self.start_y = self.y
            self.target_x = new_path[-1][0]
            self.target_y = new_path[-1][1]
            self.total_distance = len(new_path) - 1
            self.move_progress = 0
            self.is_moving = True
            self.will_hit_wall = hit_wall
            self.wall_direction = (dx, dy) if hit_wall else None
        elif len(new_path) == 1 and count > 0:
            self.handle_wall_collision(dx, dy)
    
    def handle_wall_collision(self, dx, dy):
        """Handle collision with wall"""
        current_time = pygame.time.get_ticks()
        
        # Always do bounce animation if cooldown has passed
        if current_time - self.last_bounce_time >= BOUNCE_COOLDOWN:
            self.bounce_offset_x = -dx * 5
            self.bounce_offset_y = -dy * 5
            self.bounce_duration = 12
            self.last_bounce_time = current_time
        
        # Only send rumble if enough time has passed since last rumble
        if current_time - self.last_rumble_time >= RUMBLE_COOLDOWN:
            uart.send_event("RUMBLE")
            self.last_rumble_time = current_time
            self.ignore_commands = current_time + 2000
            uart.clear_commands()

    def should_ignore(self):
        return pygame.time.get_ticks() < self.ignore_commands
    
    def update_position(self):
        if self.is_moving and self.path:
            self.move_progress += 1
            
            total_frames = self.total_distance * SMOOTH_MOVE_DURATION
            
            if self.move_progress < total_frames:
                t = self.move_progress / total_frames
                t_eased = 1 - pow(1 - t, 3)
                
                exact_distance = t_eased * self.total_distance
                
                self.display_x = self.start_x + (self.target_x - self.start_x) * t_eased
                self.display_y = self.start_y + (self.target_y - self.start_y) * t_eased
                
                if self.move_progress % 3 == 0:
                    self.trail.append((self.display_x, self.display_y))
                    if len(self.trail) > self.trail_max:
                        self.trail.pop(0)
            else:
                self.display_x = float(self.target_x)
                self.display_y = float(self.target_y)
                self.x = self.target_x
                self.y = self.target_y
                self.is_moving = False
                self.path = []
                
                if self.will_hit_wall and self.wall_direction:
                    dx, dy = self.wall_direction
                    self.handle_wall_collision(dx, dy)
                    self.will_hit_wall = False
                    self.wall_direction = None
                
                if maze[self.y][self.x] == 2:
                    return True
        
        return False
    
    def update_bounce(self):
        if self.bounce_duration > 0:
            self.bounce_duration -= 1
            self.bounce_offset_x *= 0.75
            self.bounce_offset_y *= 0.75
            if self.bounce_duration == 0:
                self.bounce_offset_x = 0
                self.bounce_offset_y = 0

def parse_uart_command(command_str):
    """Parse UART command like 'LEFT 4' into direction and count, or 'DARK 1' for theme"""
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

def draw_realistic_wall(screen, x, y):
    """Draw a wall with realistic lighting and depth"""
    pygame.draw.rect(screen, colors['WALL_SHADOW'], (x, y, CELL_SIZE, CELL_SIZE))
    pygame.draw.rect(screen, colors['WALL_BASE'], (x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2))
    
    for i in range(CELL_SIZE - 4):
        progress = i / (CELL_SIZE - 4)
        color = tuple(int(colors['WALL_FACE'][c] + (colors['WALL_LIGHT'][c] - colors['WALL_FACE'][c]) * (1 - progress)) 
                     for c in range(3))
        pygame.draw.line(screen, color, (x + 2, y + 2 + i), (x + CELL_SIZE - 2, y + 2 + i))
    
    pygame.draw.line(screen, colors['WALL_BRIGHT'], (x + 2, y + 2), (x + CELL_SIZE - 3, y + 2), 2)
    pygame.draw.line(screen, colors['WALL_LIGHT'], (x + 2, y + 4), (x + CELL_SIZE - 3, y + 4))
    pygame.draw.line(screen, colors['WALL_BRIGHT'], (x + 2, y + 2), (x + 2, y + CELL_SIZE - 3), 2)
    pygame.draw.line(screen, colors['WALL_LIGHT'], (x + 4, y + 2), (x + 4, y + CELL_SIZE - 3))
    
    pygame.draw.line(screen, colors['WALL_SHADOW'], (x + CELL_SIZE - 2, y + 4), 
                    (x + CELL_SIZE - 2, y + CELL_SIZE - 1), 2)
    pygame.draw.line(screen, colors['WALL_SHADOW'], (x + 4, y + CELL_SIZE - 2), 
                    (x + CELL_SIZE - 1, y + CELL_SIZE - 2), 2)

def draw_realistic_goal(screen, x, y, frame_count):
    """Draw goal with realistic pulsing glow"""
    pulse = abs(math.sin(frame_count * 0.1)) * 0.35 + 0.65
    
    for layer in range(4):
        size = int(CELL_SIZE * (2.0 - layer * 0.3) * pulse)
        alpha = int(70 * (1 - layer * 0.2) * pulse)
        if size > 0:
            glow_surf = pygame.Surface((size, size), pygame.SRCALPHA)
            pygame.draw.circle(glow_surf, (*colors['GOAL_GLOW'], alpha), (size // 2, size // 2), size // 2)
            screen.blit(glow_surf, (x + CELL_SIZE // 2 - size // 2, y + CELL_SIZE // 2 - size // 2))
    
    goal_size = CELL_SIZE - 6
    goal_surf = pygame.Surface((goal_size, goal_size), pygame.SRCALPHA)
    
    for i in range(goal_size):
        progress = i / goal_size
        color = tuple(int(colors['GOAL_CORE'][c] * (1 - progress * 0.3) + colors['GOAL_BRIGHT'][c] * progress * 0.3) 
                     for c in range(3))
        pygame.draw.line(goal_surf, color, (0, i), (goal_size, i))
    
    screen.blit(goal_surf, (x + 3, y + 3))
    
    center_x = x + CELL_SIZE // 2
    center_y = y + CELL_SIZE // 2
    for r in range(int(CELL_SIZE * 0.3), 0, -1):
        alpha = int(200 * (r / (CELL_SIZE * 0.3)) * pulse)
        pygame.draw.circle(screen, (*colors['GOAL_BRIGHT'], alpha), (center_x, center_y), r)
    
    highlight_size = int(CELL_SIZE * 0.15 * pulse)
    if highlight_size > 0:
        pygame.draw.circle(screen, (255, 255, 255), 
                          (center_x - 3, center_y - 3), highlight_size)

def draw_maze(screen, frame_count):
    """Draw the maze centered on screen"""
    maze_width = len(maze[0]) * CELL_SIZE
    maze_height = len(maze) * CELL_SIZE
    
    offset_x = (WINDOW_WIDTH - maze_width) // 2
    offset_y = (WINDOW_HEIGHT - maze_height) // 2
    
    for row in range(len(maze)):
        for col in range(len(maze[row])):
            x = col * CELL_SIZE + offset_x
            y = row * CELL_SIZE + offset_y
            
            if maze[row][col] == 1:
                draw_realistic_wall(screen, x, y)
            elif maze[row][col] == 2:
                draw_realistic_goal(screen, x, y, frame_count)
            else:
                color = colors['PATH_ALT'] if (row + col) % 2 == 0 else colors['PATH_BASE']
                pygame.draw.rect(screen, color, (x, y, CELL_SIZE, CELL_SIZE))
            
            pygame.draw.rect(screen, colors['GRID_COLOR'], (x, y, CELL_SIZE, CELL_SIZE), 1)
    
    return offset_x, offset_y

def draw_realistic_player(screen, x, y, frame_count, trail, offset_x, offset_y):
    """Draw player as a realistic 3D sphere with lighting"""
    center_x = int(x + CELL_SIZE // 2)
    center_y = int(y + CELL_SIZE // 2)
    radius = CELL_SIZE // 3
    
    for i, (tx, ty) in enumerate(trail):
        alpha = int(140 * ((i + 1) / len(trail)))
        size = int(radius * 0.7 * ((i + 1) / len(trail)))
        
        if size > 0:
            trail_surf = pygame.Surface((size * 3, size * 3), pygame.SRCALPHA)
            
            for r in range(size, 0, -1):
                particle_alpha = int(alpha * (r / size) * 0.8)
                pygame.draw.circle(trail_surf, (*colors['PLAYER_LIGHT'], particle_alpha), 
                                 (size * 3 // 2, size * 3 // 2), r)
            
            trail_x = int(tx * CELL_SIZE + offset_x + CELL_SIZE // 2 - size * 1.5)
            trail_y = int(ty * CELL_SIZE + offset_y + CELL_SIZE // 2 - size * 1.5)
            screen.blit(trail_surf, (trail_x, trail_y))
    
    shadow_surf = pygame.Surface((int(CELL_SIZE * 1.2), int(CELL_SIZE * 0.6)), pygame.SRCALPHA)
    for i in range(int(CELL_SIZE * 0.6), 0, -2):
        alpha = int(colors['SHADOW_ALPHA'] * (i / (CELL_SIZE * 0.6)))
        pygame.draw.ellipse(shadow_surf, (0, 0, 0, alpha), 
                          (int(CELL_SIZE * 0.1), int(CELL_SIZE * 0.3 - i / 2), 
                           int(CELL_SIZE), i))
    screen.blit(shadow_surf, (center_x - int(CELL_SIZE * 0.6), center_y + radius // 2))
    
    pulse = abs(math.sin(frame_count * 0.08)) * 0.2 + 0.8
    for layer in range(4):
        glow_radius = int((radius * 2.5 - layer * 8) * pulse)
        alpha = int(45 - layer * 8)
        if glow_radius > 0 and alpha > 0:
            glow_surf = pygame.Surface((glow_radius * 2, glow_radius * 2), pygame.SRCALPHA)
            pygame.draw.circle(glow_surf, (*colors['PLAYER_LIGHT'], alpha), 
                             (glow_radius, glow_radius), glow_radius)
            screen.blit(glow_surf, (center_x - glow_radius, center_y - glow_radius))
    
    pygame.draw.circle(screen, (20, 90, 180), (center_x + 1, center_y + 1), radius + 2)
    pygame.draw.circle(screen, (25, 100, 200), (center_x, center_y), radius + 2)
    
    for r in range(radius, 0, -1):
        progress = 1 - (r / radius)
        color = tuple(int(colors['PLAYER_CORE'][c] + (colors['PLAYER_MID'][c] - colors['PLAYER_CORE'][c]) * progress) 
                     for c in range(3))
        pygame.draw.circle(screen, color, (center_x, center_y), r)
    
    light_offset = -radius // 3
    light_radius = int(radius * 0.9)
    if light_radius > 0:
        for r in range(light_radius, 0, -1):
            alpha = int(180 * (r / light_radius))
            light_surf = pygame.Surface((r * 2, r * 2), pygame.SRCALPHA)
            pygame.draw.circle(light_surf, (*colors['PLAYER_LIGHT'], alpha), (r, r), r)
            screen.blit(light_surf, (center_x + light_offset - r, center_y + light_offset - r))
    
    spec_size = max(radius // 2, 1)
    spec_x = center_x - radius // 2
    spec_y = center_y - radius // 2
    
    if spec_size > 0:
        for r in range(spec_size, 0, -1):
            alpha = int(255 * (r / spec_size))
            pygame.draw.circle(screen, (*colors['PLAYER_SHINE'], alpha), (spec_x, spec_y), r)
        
        highlight_size = max(spec_size // 3, 1)
        pygame.draw.circle(screen, (255, 255, 255), (spec_x, spec_y), highlight_size)

def score_calculation(timer, prev_high_score):
    if prev_high_score == 0:
        new_high_score = timer
    elif timer < prev_high_score:
        new_high_score = timer
    else: 
        new_high_score = prev_high_score
    return new_high_score

def reset_game_state(player):
    """Reset all game state variables"""
    player.__init__(1, 1)
    return 0, 0, 0, 0, None

def handle_win_gesture(win_gesture_type, win_gesture_count, gesture_type):
    """Handle win screen gesture logic. Returns (new_type, new_count)"""
    if win_gesture_type == gesture_type:
        return gesture_type, win_gesture_count + 1
    else:
        return gesture_type, 1

def game_loop(screen, clock, high_score):
    uart.start_polling()
    
    player = Player(1, 1)
    font = pygame.font.Font(None, 32)
    small_font = pygame.font.Font(None, 18)
    timer_font = pygame.font.Font(None, 36)
    prev_high_score = high_score
    
    won = False
    frame_count = 0
    timer_start = pygame.time.get_ticks()
    final_time = 0
    pause_time_accumulated = 0
    win_gesture_count = 0
    win_gesture_type = None
    
    try:
        while True:
            frame_count += 1
            current_time = pygame.time.get_ticks()
            
            # Event handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                
                # Keyboard controls during gameplay
                if event.type == pygame.KEYDOWN and not won:
                    if event.key == pygame.K_p:
                        game_snapshot = screen.copy()
                        pause_start = pygame.time.get_ticks()
                        pause_result = pause_screen(screen, clock, game_snapshot)
                        pause_time_accumulated += pygame.time.get_ticks() - pause_start
                        
                        if pause_result == "restart":
                            frame_count, timer_start, final_time, pause_time_accumulated, _ = reset_game_state(player)
                            win_gesture_count = 0
                            win_gesture_type = None
                            won = False
                            uart.clear_commands()
                        elif pause_result == "menu":
                            return "menu"
                    
                    elif event.key == pygame.K_ESCAPE:
                        return "menu"
                    
                    elif event.key == pygame.K_w:  # Debug key to trigger win screen
                        won = True
                        uart.send_event("VIC")
                        final_time = (pygame.time.get_ticks() - timer_start - pause_time_accumulated) / 1000
                
                # Keyboard controls on win screen - A and D for gestures
                if event.type == pygame.KEYDOWN and won:
                    if event.key == pygame.K_a:  # A = LEFT gesture
                        win_gesture_type, win_gesture_count = handle_win_gesture(
                            win_gesture_type, win_gesture_count, "LEFT"
                        )
                        if win_gesture_count >= 10:
                            return "menu"
                    
                    elif event.key == pygame.K_d:  # D = RIGHT gesture
                        win_gesture_type, win_gesture_count = handle_win_gesture(
                            win_gesture_type, win_gesture_count, "RIGHT"
                        )
                        if win_gesture_count >= 10:
                            uart.send_event("LEVEL")
                            return "level2"
                    
                    elif event.key == pygame.K_r:
                        frame_count, timer_start, final_time, pause_time_accumulated, _ = reset_game_state(player)
                        win_gesture_count = 0
                        win_gesture_type = None
                        won = False
                        uart.clear_commands()
                    
                    elif event.key == pygame.K_m:
                        return "menu"
                    
                    elif event.key == pygame.K_n:
                        uart.send_event("LEVEL")
                        return "level2"
                    
                    # Any other key resets gesture counter
                    elif event.key not in [pygame.K_a, pygame.K_d, pygame.K_r, pygame.K_m, pygame.K_n]:
                        win_gesture_count = 0
                        win_gesture_type = None
            
            # Process UART commands
            command_str = uart.get_command()
            
            if command_str:
                command, value = parse_uart_command(command_str)
                
                # Handle PAUSE command
                if command == "PAUSE":
                    game_snapshot = screen.copy()
                    pause_start = pygame.time.get_ticks()
                    pause_result = pause_screen(screen, clock, game_snapshot)
                    pause_time_accumulated += pygame.time.get_ticks() - pause_start
                    
                    if pause_result == "restart":
                        frame_count, timer_start, final_time, pause_time_accumulated, _ = reset_game_state(player)
                        win_gesture_count = 0
                        win_gesture_type = None
                        won = False
                        uart.clear_commands()
                    elif pause_result == "menu":
                        return "menu"
                
                # Handle DARK theme command
                elif command == "DARK":
                    set_theme(value == 1)
                
                # Handle gestures on win screen
                elif won:
                    if command in ["LEFT", "RIGHT"]:
                        win_gesture_type, win_gesture_count = handle_win_gesture(
                            win_gesture_type, win_gesture_count, command
                        )
                        
                        # Check if we've reached 3 consecutive gestures
                        if win_gesture_count >= 10:
                            if command == "LEFT":
                                return "menu"
                            elif command == "RIGHT":
                                return "level2"
                    else:
                        # Non-LEFT/RIGHT command resets the gesture counter
                        win_gesture_count = 0
                        win_gesture_type = None
                
                # Handle movement commands during gameplay
                elif not player.is_moving and not won:
                    if player.should_ignore():
                        pass
                    if command == "UP":
                        player.queue_moves(0, -1, value)
                    elif command == "DOWN":
                        player.queue_moves(0, 1, value)
                    elif command == "LEFT":
                        player.queue_moves(-1, 0, value)
                    elif command == "RIGHT":
                        player.queue_moves(1, 0, value)
            
            # Update player position and check for win
            if player.update_position() and not won:
                won = True
                uart.send_event("VIC")
                final_time = (pygame.time.get_ticks() - timer_start - pause_time_accumulated) / 1000
                new_high_score = score_calculation(final_time, prev_high_score)
            
            player.update_bounce()
            
            # Drawing
            screen.fill(colors['PATH_ALT'])
            offset_x, offset_y = draw_maze(screen, frame_count)
            
            player_screen_x = player.display_x * CELL_SIZE + offset_x + player.bounce_offset_x
            player_screen_y = player.display_y * CELL_SIZE + offset_y + player.bounce_offset_y
            
            draw_realistic_player(screen, player_screen_x, player_screen_y, 
                                frame_count, player.trail, offset_x, offset_y)
            
            # Display timer
            if won:
                elapsed_time = final_time
            else:
                elapsed_time = (pygame.time.get_ticks() - timer_start - pause_time_accumulated) / 1000
            
            timer_text = timer_font.render(f"Time: {elapsed_time:.1f}s", True, colors['TEXT_COLOR'])
            timer_rect = timer_text.get_rect(topright=(WINDOW_WIDTH - 20, 20))
            
            # Timer background
            padding = 10
            bg_rect = pygame.Rect(timer_rect.left - padding, timer_rect.top - padding,
                                 timer_rect.width + padding * 2, timer_rect.height + padding * 2)
            
            timer_bg_color = (35, 40, 50) if current_theme == 'dark' else (245, 248, 252)
            timer_border_color = (70, 80, 100) if current_theme == 'dark' else (180, 190, 205)
            
            pygame.draw.rect(screen, timer_bg_color, bg_rect, border_radius=8)
            pygame.draw.rect(screen, timer_border_color, bg_rect, 2, border_radius=8)
            screen.blit(timer_text, timer_rect)
            
            # Win screen overlay
            if won:
                overlay = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT))
                overlay.set_alpha(230)
                overlay.fill(colors['PATH_ALT'])
                screen.blit(overlay, (0, 0))
                
                win_text = font.render("You Won!", True, colors['GOAL_CORE'])
                
                # Show gesture progress indicator
                if win_gesture_count > 0 and win_gesture_type:
                    if win_gesture_type == "LEFT":
                        gesture_indicator = "◀ " * win_gesture_count
                        direction_text = "Menu"
                    else:
                        gesture_indicator = "▶ " * win_gesture_count
                        direction_text = "Next Level"
                    
                    gesture_text = small_font.render(
                        f"{gesture_indicator}({win_gesture_count}/10) → {direction_text}", 
                        True, colors['TEXT_COLOR']
                    )
                    gesture_rect = gesture_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2 + 50))
                    
                    # Highlight background for gesture indicator
                    indicator_padding = 8
                    indicator_bg_rect = pygame.Rect(
                        gesture_rect.left - indicator_padding,
                        gesture_rect.top - indicator_padding,
                        gesture_rect.width + indicator_padding * 2,
                        gesture_rect.height + indicator_padding * 2
                    )
                    indicator_bg_color = (100, 200, 100) if win_gesture_type == "RIGHT" else (200, 100, 100)
                    indicator_bg_color = (*indicator_bg_color, 180)
                    
                    indicator_surf = pygame.Surface((indicator_bg_rect.width, indicator_bg_rect.height), pygame.SRCALPHA)
                    pygame.draw.rect(indicator_surf, indicator_bg_color, indicator_surf.get_rect(), border_radius=6)
                    screen.blit(indicator_surf, (indicator_bg_rect.x, indicator_bg_rect.y))
                    screen.blit(gesture_text, gesture_rect)
                
                restart_text = small_font.render(
                    "10x A: Menu | 10x D: Next Level | R: Restart", 
                    True, colors['TEXT_COLOR']
                )
                
                text_rect = win_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2))
                restart_rect = restart_text.get_rect(center=(WINDOW_WIDTH // 2, WINDOW_HEIGHT // 2 + 75))
                
                # Win text shadow
                shadow_color = (0, 0, 0) if current_theme == 'light' else (10, 10, 10)
                shadow_text = font.render("You Won!", True, shadow_color)
                shadow_rect = shadow_text.get_rect(center=(WINDOW_WIDTH // 2 + 2, WINDOW_HEIGHT // 2 + 2))
                
                screen.blit(shadow_text, shadow_rect)
                screen.blit(win_text, text_rect)
                screen.blit(restart_text, restart_rect)
            
            pygame.display.flip()
            clock.tick(FPS)
    
    finally:
        uart.stop_polling()