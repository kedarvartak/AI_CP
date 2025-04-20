#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <stdbool.h> // Added for bool type
#include "include/raylib.h" // Added for Raylib

#define SIZE 3

// --- Raylib Specific Defines ---
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 650 // Increased height for UI text
#define CELL_SIZE 150
#define GRID_THICKNESS 5
#define PADDING 50 // Padding around the grid
#define SYMBOL_PADDING (CELL_SIZE * 0.2f) // Padding inside cell for symbols
#define SYMBOL_THICKNESS (CELL_SIZE * 0.1f)

#define GRID_COLOR DARKGRAY
#define X_COLOR BLUE
#define O_COLOR RED
#define WIN_COLOR LIME
#define HOVER_COLOR Fade(YELLOW, 0.3f)
#define BACKGROUND_COLOR RAYWHITE

char board[SIZE][SIZE];
int difficulty = 3; // Default to hard
char playerSymbol = ' '; // Player selects X or O
char aiSymbol = ' ';

// Game State Enum
typedef enum {
    SELECT_SYMBOL,      // New state for symbol selection
    SELECT_DIFFICULTY,
    PLAYER_TURN,
    AI_TURN,
    GAME_OVER
} GameScreen;

// --- Function Declarations ---
void InitializeBoard();
// void printBoard(); // Replaced by DrawBoard2D
int IsMovesLeft();
char CheckWinner();
int Evaluate();
int AlphaBeta(int depth, bool isMax, int alpha, int beta, int maxDepth); // Changed isMax to bool
void FindBestMove(int *bestRow, int *bestCol);
void MakeRandomMove(int *row, int *col);
void AIMove();
void DrawBoard2D(int hoverRow, int hoverCol); // Added hover parameters
void DrawUI(GameScreen currentScreen, char winner, Font font); // Added UI function

// --- Game Logic Functions (Mostly Unchanged) ---

void InitializeBoard() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';
}

int IsMovesLeft() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == ' ')
                return 1;
    return 0;
}

char CheckWinner() {
    // Rows and Columns
    for (int i = 0; i < SIZE; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return board[i][0];
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return board[0][i];
    }
    // Diagonals
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return board[0][0];
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return board[0][2];

    return ' '; // No winner
}

int Evaluate() {
    char winner = CheckWinner();
    if (winner == aiSymbol) return 10;      // Use aiSymbol
    else if (winner == playerSymbol) return -10; // Use playerSymbol
    return 0; // Draw or ongoing
}

// Minimax with Alpha-Beta Pruning (changed isMax to bool)
int AlphaBeta(int depth, bool isMax, int alpha, int beta, int maxDepth) {
    int score = Evaluate();

    // Terminal states
    if (score == 10) return score - depth; // AI wins
    if (score == -10) return score + depth; // Human wins
    if (!IsMovesLeft() || depth >= maxDepth) return 0; // Draw or depth limit

    if (isMax) { // AI's turn (Maximizer)
        int best = INT_MIN;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = aiSymbol; // Use aiSymbol
                    int val = AlphaBeta(depth + 1, false, alpha, beta, maxDepth);
                    best = (best > val) ? best : val;
                    alpha = (alpha > best) ? alpha : best;
                    board[i][j] = ' '; // Undo move
                    if (beta <= alpha) return best; // Prune
                }
            }
        }
        return best;
    } else { // Human's turn (Minimizer)
        int best = INT_MAX;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = playerSymbol; // Use playerSymbol
                    int val = AlphaBeta(depth + 1, true, alpha, beta, maxDepth);
                    best = (best < val) ? best : val;
                    beta = (beta < best) ? beta : best;
                    board[i][j] = ' '; // Undo move
                    if (beta <= alpha) return best; // Prune
                }
            }
        }
        return best;
    }
}

void FindBestMove(int *bestRow, int *bestCol) {
    int bestVal = INT_MIN;
    *bestRow = -1;
    *bestCol = -1;
    // Adjust maxDepth based on difficulty (more moves explored for harder levels)
    int maxDepth = (difficulty == 1) ? 1 : (difficulty == 2) ? 3 : 9;

    // --- Randomness for equivalent best moves ---
    typedef struct { int r, c; } Move;
    Move bestMoves[SIZE*SIZE];
    int numBestMoves = 0;
    // --- End Randomness ---


    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ') {
                board[i][j] = aiSymbol; // Use aiSymbol
                // Use AlphaBeta directly here for evaluation
                int moveVal = AlphaBeta(0, false, INT_MIN, INT_MAX, maxDepth);
                board[i][j] = ' '; // Undo move

                if (moveVal > bestVal) {
                    bestVal = moveVal;
                    // Reset best moves list
                    numBestMoves = 0;
                    bestMoves[numBestMoves++] = (Move){i, j};
                } else if (moveVal == bestVal) {
                    // Add to list of equally good moves
                    if (numBestMoves < SIZE*SIZE) { // Basic bounds check
                         bestMoves[numBestMoves++] = (Move){i, j};
                    }
                }
            }
        }
    }

    // Select randomly from the best moves found
    if (numBestMoves > 0) {
        int randomIndex = rand() % numBestMoves;
        *bestRow = bestMoves[randomIndex].r;
        *bestCol = bestMoves[randomIndex].c;
    } else {
        // Fallback: if no move evaluated (shouldn't happen if moves left), pick first available
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    *bestRow = i;
                    *bestCol = j;
                    return;
                }
            }
        }
    }
}


void MakeRandomMove(int *row, int *col) {
    int emptyCells[SIZE * SIZE][2], count = 0;
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == ' ') {
                emptyCells[count][0] = i;
                emptyCells[count][1] = j;
                count++;
            }
    if (count > 0) {
        int choice = rand() % count;
        *row = emptyCells[choice][0];
        *col = emptyCells[choice][1];
    } else {
        *row = -1; // Indicate no move possible
        *col = -1;
    }
}

void AIMove() {
    int row = -1, col = -1;
    if (difficulty == 1) { // Easy: Random move
        MakeRandomMove(&row, &col);
    } else { // Medium/Hard: Use Minimax (AlphaBeta)
        FindBestMove(&row, &col);
    }

    // Ensure a valid move was found before placing
    if (row != -1 && col != -1 && board[row][col] == ' ') {
        board[row][col] = aiSymbol; // Use aiSymbol
    } else if (IsMovesLeft()) {
        // Fallback if AI logic failed but moves are available (should not happen often)
        printf("AI Logic Error: Making random fallback move.\n"); // Corrected newline escape
        MakeRandomMove(&row, &col);
        if (row != -1 && col != -1) {
             board[row][col] = aiSymbol; // Use aiSymbol
        }
    }
}

// --- Raylib Drawing Functions ---

void DrawBoard2D(int hoverRow, int hoverCol) {
    // Draw grid lines
    for (int i = 1; i < SIZE; i++) {
        // Vertical lines
        DrawRectangle(PADDING + i * CELL_SIZE - GRID_THICKNESS / 2, PADDING, GRID_THICKNESS, SIZE * CELL_SIZE, GRID_COLOR);
        // Horizontal lines
        DrawRectangle(PADDING, PADDING + i * CELL_SIZE - GRID_THICKNESS / 2, SIZE * CELL_SIZE, GRID_THICKNESS, GRID_COLOR);
    }

    // Draw symbols and hover effect
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            float cellX = PADDING + j * CELL_SIZE;
            float cellY = PADDING + i * CELL_SIZE;

            // Draw hover highlight
            if (i == hoverRow && j == hoverCol && board[i][j] == ' ') {
                DrawRectangle(cellX, cellY, CELL_SIZE, CELL_SIZE, HOVER_COLOR);
            }

            // Draw X or O
            if (board[i][j] == 'X') {
                // Draw two lines for X
                DrawLineEx((Vector2){cellX + SYMBOL_PADDING, cellY + SYMBOL_PADDING},
                           (Vector2){cellX + CELL_SIZE - SYMBOL_PADDING, cellY + CELL_SIZE - SYMBOL_PADDING},
                           SYMBOL_THICKNESS, X_COLOR);
                DrawLineEx((Vector2){cellX + SYMBOL_PADDING, cellY + CELL_SIZE - SYMBOL_PADDING},
                           (Vector2){cellX + CELL_SIZE - SYMBOL_PADDING, cellY + SYMBOL_PADDING},
                           SYMBOL_THICKNESS, X_COLOR);
            } else if (board[i][j] == 'O') {
                // Draw circle for O using DrawRing for a thicker appearance
                float centerX = cellX + CELL_SIZE / 2.0f;
                float centerY = cellY + CELL_SIZE / 2.0f;
                float outerRadius = CELL_SIZE / 2.0f - SYMBOL_PADDING / 2.0f;
                float innerRadius = outerRadius - SYMBOL_THICKNESS; // Make the ring thickness match X thickness
                if (innerRadius < 0) innerRadius = 0; // Ensure inner radius is not negative

                DrawRing((Vector2){centerX, centerY}, innerRadius, outerRadius, 0, 360, 36, O_COLOR);
            }
        }
    }

     // Draw winning line (Optional - simplified version)
     char winner = CheckWinner(); // Re-check needed if not passed
     if (winner != ' ') {
         // Basic win line logic (can be expanded like 3D version)
         // Find the winning line start/end cells and draw a thick line
         // This part requires storing the winning line coords like in 3D version
         // For simplicity, we'll skip detailed line drawing for now.
     }
}

void DrawUI(GameScreen currentScreen, char winner, Font font) {
    int fontSize = 20;
    int textY = PADDING + SIZE * CELL_SIZE + 20; // Position UI below the grid

    switch(currentScreen) {
        case SELECT_SYMBOL:
            DrawTextEx(font, "SELECT YOUR SYMBOL:", (Vector2){PADDING, textY}, fontSize * 1.5, 1.5, BLACK);
            DrawTextEx(font, "Press [X] or [O]", (Vector2){PADDING, textY + 40}, fontSize, 1.5, DARKGRAY);
            break;
        case SELECT_DIFFICULTY:
            DrawTextEx(font, "SELECT DIFFICULTY:", (Vector2){PADDING, textY}, fontSize * 1.5, 1.5, BLACK);
            DrawTextEx(font, "[1] Easy  [2] Medium  [3] Hard", (Vector2){PADDING, textY + 40}, fontSize, 1.5, DARKGRAY);
            break;
        case PLAYER_TURN:
            DrawTextEx(font, TextFormat("PLAYER (%c) TURN", playerSymbol), (Vector2){PADDING, textY}, fontSize, 1.5, BLACK);
            break;
        case AI_TURN:
            DrawTextEx(font, TextFormat("AI (%c) THINKING...", aiSymbol), (Vector2){PADDING, textY}, fontSize, 1.5, BLACK);
            break;
        case GAME_OVER:
            {
                const char* resultText;
                Color resultColor;
                if (winner == playerSymbol) { resultText = "YOU WIN!"; resultColor = (playerSymbol == 'X' ? X_COLOR : O_COLOR); }
                else if (winner == aiSymbol) { resultText = "AI WINS!"; resultColor = (aiSymbol == 'X' ? X_COLOR : O_COLOR); }
                else { resultText = "IT'S A DRAW!"; resultColor = GRAY; }

                Vector2 textSize = MeasureTextEx(font, resultText, fontSize * 2, 1.5);
                DrawTextEx(font, resultText, (Vector2){(SCREEN_WIDTH - textSize.x) / 2, textY}, fontSize * 2, 1.5, resultColor);
                DrawTextEx(font, "Press [R] to Restart", (Vector2){(SCREEN_WIDTH - MeasureText("Press [R] to Restart", fontSize)) / 2, textY + 50}, fontSize, 1.5, DARKGRAY);
            }
            break;
    }
     // Display difficulty only after it's selected
     if (currentScreen != SELECT_SYMBOL && currentScreen != SELECT_DIFFICULTY) {
        DrawTextEx(font, TextFormat("Difficulty: %d", difficulty), (Vector2){PADDING, 15}, fontSize, 1.5, GRAY);
     }
     // Display selected symbols after selection
     if (currentScreen != SELECT_SYMBOL) {
         DrawTextEx(font, TextFormat("Player: %c | AI: %c", playerSymbol, aiSymbol), (Vector2){SCREEN_WIDTH - PADDING - 150, 15}, fontSize, 1.5, GRAY);
     }
}


// --- Main Game Function (Raylib Version) ---
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Tic-Tac-Toe - Raylib");
    SetTargetFPS(60);
    srand(time(NULL)); // Seed random number generator

    // InitializeBoard(); // Board initialized after symbol/difficulty selection
    char winner = ' ';
    GameScreen currentScreen = SELECT_SYMBOL; // Start with symbol selection
    Font font = GetFontDefault(); // Use default Raylib font

    int hoverRow = -1;
    int hoverCol = -1;
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();
        hoverRow = -1; // Reset hover state each frame
        hoverCol = -1;

        // --- Input and State Handling ---
        switch (currentScreen) {
            case SELECT_SYMBOL:
                if (IsKeyPressed(KEY_X)) {
                    playerSymbol = 'X';
                    aiSymbol = 'O';
                    currentScreen = SELECT_DIFFICULTY;
                }
                if (IsKeyPressed(KEY_O)) {
                    playerSymbol = 'O';
                    aiSymbol = 'X';
                    currentScreen = SELECT_DIFFICULTY;
                }
                break;

            case SELECT_DIFFICULTY:
                if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) { difficulty = 1; InitializeBoard(); winner = ' '; currentScreen = (playerSymbol == 'X' ? PLAYER_TURN : AI_TURN); }
                if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) { difficulty = 2; InitializeBoard(); winner = ' '; currentScreen = (playerSymbol == 'X' ? PLAYER_TURN : AI_TURN); }
                if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) { difficulty = 3; InitializeBoard(); winner = ' '; currentScreen = (playerSymbol == 'X' ? PLAYER_TURN : AI_TURN); }
                break;

            case PLAYER_TURN:
                // Calculate hovered cell
                if (CheckCollisionPointRec(mousePos, (Rectangle){PADDING, PADDING, SIZE * CELL_SIZE, SIZE * CELL_SIZE})) {
                    hoverCol = (int)(mousePos.x - PADDING) / CELL_SIZE;
                    hoverRow = (int)(mousePos.y - PADDING) / CELL_SIZE;

                    // Clamp values just in case
                    if (hoverRow < 0) hoverRow = 0; if (hoverRow >= SIZE) hoverRow = SIZE - 1;
                    if (hoverCol < 0) hoverCol = 0; if (hoverCol >= SIZE) hoverCol = SIZE - 1;

                    // Check for click
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && board[hoverRow][hoverCol] == ' ') {
                        board[hoverRow][hoverCol] = playerSymbol; // Use playerSymbol
                        winner = CheckWinner();
                        if (winner != ' ' || !IsMovesLeft()) {
                            currentScreen = GAME_OVER;
                        } else {
                            currentScreen = AI_TURN; // Switch to AI turn
                        }
                    }
                }
                break;

            case AI_TURN:
                // AI makes its move
                AIMove();
                winner = CheckWinner();
                if (winner != ' ' || !IsMovesLeft()) {
                    currentScreen = GAME_OVER;
                } else {
                    currentScreen = PLAYER_TURN; // Switch back to Player turn
                }
                break;

            case GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    // Reset game state completely
                    playerSymbol = ' ';
                    aiSymbol = ' ';
                    InitializeBoard(); // Clear board
                    winner = ' ';
                    currentScreen = SELECT_SYMBOL; // Go back to symbol selection
                }
                break;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);

            // Only draw board if symbols have been selected
            if (currentScreen != SELECT_SYMBOL) {
                DrawBoard2D(hoverRow, hoverCol); // Draw the game board
            }
            DrawUI(currentScreen, winner, font); // Draw UI elements

            // DrawFPS(10, 10); // Optional: Show FPS

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

/* // Old main function (for reference)
int main() {
    srand(time(NULL));
    printf("Welcome to 2D Tic-Tac-Toe with Difficulty Levels!
");
    printf("Select difficulty:
1. Easy
2. Medium
3. Hard
Choice: ");
    scanf("%d", &difficulty);
    if (difficulty < 1 || difficulty > 3) {
        printf("Invalid choice! Defaulting to Hard.
");
        difficulty = 3;
    }
    playGame();
    return 0;
}
*/