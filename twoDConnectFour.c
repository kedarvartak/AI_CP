#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "include/raylib.h" // Include Raylib

// Define constants
#define ROWS 6
#define COLS 7
#define PLAYER 1
#define AI 2
#define EMPTY 0

// Raylib drawing constants
#define SCREEN_WIDTH 700
#define SCREEN_HEIGHT 700 // Increased height for messages
#define CELL_SIZE 100
#define PADDING 5
#define PIECE_RADIUS (CELL_SIZE / 2 - PADDING)
#define BOARD_OFFSET_X 0
#define BOARD_OFFSET_Y 100 // Offset board down to make space for messages

// Global variable for difficulty (could be made local)
int difficulty = 4; // Default difficulty

// Define the 2D board
int board[ROWS][COLS];

// Game state enum
typedef enum {
    DIFFICULTY_SELECTION, // Added state
    PLAYER_TURN,
    AI_TURN,
    GAME_OVER
} GameState;

// Global game state variables
GameState currentState;
bool gameOver;
char message[150]; // Increased size for restart message
int winner;

// Forward declarations for functions used before definition
bool isFull2D();
void drawBoardRaylib(const char* message, GameState currentState); // Updated signature
void drawDifficultySelection(); // New drawing function for selection screen
void resetGame(); // Added forward declaration
int getBestMove2D();
int minimax2D(int depth, int alpha, int beta, bool maximizing);
int evaluateBoard2D();
bool winningMove2D(int piece);
void undoMove2D(int col);
int makeMove2D(int col, int piece);
bool isValidMove2D(int col);


// ----------------------- 2D CONNECT 4 FUNCTIONS -----------------------

bool isValidMove2D(int col) {
    return col >= 0 && col < COLS && board[0][col] == EMPTY;
}

int makeMove2D(int col, int piece) {
    for (int r = ROWS - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY) {
            board[r][col] = piece;
            return r;
        }
    }
    return -1;
}

void undoMove2D(int col) {
    for (int r = 0; r < ROWS; r++) {
        if (board[r][col] != EMPTY) {
            board[r][col] = EMPTY;
            break;
        }
    }
}

bool winningMove2D(int piece) {
    // Horizontal
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS - 3; c++)
            if (board[r][c] == piece && board[r][c+1] == piece && board[r][c+2] == piece && board[r][c+3] == piece)
                return true;
    // Vertical
    for (int c = 0; c < COLS; c++)
        for (int r = 0; r < ROWS - 3; r++)
            if (board[r][c] == piece && board[r+1][c] == piece && board[r+2][c] == piece && board[r+3][c] == piece)
                return true;
    // Positive Diagonal
    for (int r = 0; r < ROWS - 3; r++)
        for (int c = 0; c < COLS - 3; c++)
            if (board[r][c] == piece && board[r+1][c+1] == piece && board[r+2][c+2] == piece && board[r+3][c+3] == piece)
                return true;
    // Negative Diagonal
    for (int r = 3; r < ROWS; r++)
        for (int c = 0; c < COLS - 3; c++)
            if (board[r][c] == piece && board[r-1][c+1] == piece && board[r-2][c+2] == piece && board[r-3][c+3] == piece)
                return true;
    return false;
}

int evaluateBoard2D() {
    if (winningMove2D(AI)) return 100;
    if (winningMove2D(PLAYER)) return -100;
    return 0;
}

int minimax2D(int depth, int alpha, int beta, bool maximizing) {
    if (winningMove2D(PLAYER)) return -100 - depth;
    if (winningMove2D(AI)) return 100 + depth;
    if (isFull2D()) return 0;
    if (depth == 0) return evaluateBoard2D();

    if (maximizing) {
        int maxEval = INT_MIN;
        for (int c = 0; c < COLS; c++) {
            if (isValidMove2D(c)) {
                makeMove2D(c, AI);
                int eval = minimax2D(depth - 1, alpha, beta, false);
                undoMove2D(c);
                maxEval = eval > maxEval ? eval : maxEval;
                alpha = alpha > eval ? alpha : eval;
                if (beta <= alpha)
                    break;
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for (int c = 0; c < COLS; c++) {
            if (isValidMove2D(c)) {
                makeMove2D(c, PLAYER);
                int eval = minimax2D(depth - 1, alpha, beta, true);
                undoMove2D(c);
                minEval = eval < minEval ? eval : minEval;
                beta = beta < eval ? beta : eval;
                if (beta <= alpha)
                    break;
            }
        }
        return minEval;
    }
}

int getBestMove2D() {
    int bestScore = INT_MIN;
    int bestCol = -1;

    // Prioritize center column slightly if available initially (simple heuristic)
    if (isValidMove2D(COLS / 2)) {
        bestCol = COLS / 2;
    }

    for (int c = 0; c < COLS; c++) {
        if (isValidMove2D(c)) {
            // Check for immediate AI win
            makeMove2D(c, AI);
            if (winningMove2D(AI)) {
                undoMove2D(c);
                return c; // Immediate win is the best move
            }
            undoMove2D(c);

            // Evaluate the move using minimax
            makeMove2D(c, AI);
            // We call minimax for the minimizing player (false) because it's evaluating the state *after* AI moves,
            // anticipating the player's response.
            int score = minimax2D(difficulty, INT_MIN, INT_MAX, false);
            undoMove2D(c);

            // Update best move found so far
            if (bestCol == -1 || score > bestScore) { // If it's the first valid move checked OR score is better
                bestScore = score;
                bestCol = c;
            }
        }
    }

    // If no move was found (should only happen if board is full at start, which is impossible)
    // or if all moves lead to immediate loss (minimax returns very low scores),
    // pick the first valid one found initially or during the loop.
    // If bestCol is still -1 after the loop, find the first available column as a fallback.
    if (bestCol == -1) {
        for (int c = 0; c < COLS; c++) {
            if (isValidMove2D(c)) {
                bestCol = c;
                break;
            }
        }
    }

    return bestCol;
}

bool isFull2D() {
    for (int c = 0; c < COLS; c++)
        if (board[0][c] == EMPTY)
            return false;
    return true;
}

// ----------------------- RAYLIB DRAWING FUNCTIONS -----------------------

void drawDifficultySelection() {
    ClearBackground(RAYWHITE);
    DrawText("Choose Difficulty:", SCREEN_WIDTH / 2 - MeasureText("Choose Difficulty:", 40) / 2, SCREEN_HEIGHT / 2 - 100, 40, BLACK);

    // Define button rectangles
    Rectangle easyButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 30, 200, 50 };
    Rectangle mediumButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 30, 200, 50 };
    Rectangle hardButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 90, 200, 50 };

    // Draw buttons
    DrawRectangleRec(easyButton, LIGHTGRAY);
    DrawRectangleRec(mediumButton, LIGHTGRAY);
    DrawRectangleRec(hardButton, LIGHTGRAY);

    // Draw button text
    DrawText("Easy (1)", easyButton.x + easyButton.width / 2 - MeasureText("Easy (1)", 20) / 2, easyButton.y + easyButton.height / 2 - 10, 20, BLACK);
    DrawText("Medium (2)", mediumButton.x + mediumButton.width / 2 - MeasureText("Medium (2)", 20) / 2, mediumButton.y + mediumButton.height / 2 - 10, 20, BLACK);
    DrawText("Hard (3)", hardButton.x + hardButton.width / 2 - MeasureText("Hard (3)", 20) / 2, hardButton.y + hardButton.height / 2 - 10, 20, BLACK);

    // Add hover effect (optional)
    Vector2 mousePoint = GetMousePosition();
    if (CheckCollisionPointRec(mousePoint, easyButton)) DrawRectangleLinesEx(easyButton, 2, DARKGRAY);
    if (CheckCollisionPointRec(mousePoint, mediumButton)) DrawRectangleLinesEx(mediumButton, 2, DARKGRAY);
    if (CheckCollisionPointRec(mousePoint, hardButton)) DrawRectangleLinesEx(hardButton, 2, DARKGRAY);
}

void drawBoardRaylib(const char* message, GameState currentState) { // Added currentState parameter
    // Draw board grid and pieces only if not selecting difficulty
    if (currentState != DIFFICULTY_SELECTION) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                int x = BOARD_OFFSET_X + c * CELL_SIZE;
                int y = BOARD_OFFSET_Y + r * CELL_SIZE;

                // Draw cell background (e.g., blue)
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, BLUE);
                // Draw empty slot (circle)
                DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, PIECE_RADIUS, LIGHTGRAY);

                // Draw pieces
                if (board[r][c] == PLAYER) {
                    DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, PIECE_RADIUS, RED);
                } else if (board[r][c] == AI) {
                    DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, PIECE_RADIUS, YELLOW);
                }
                 // Draw grid lines
                DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, DARKBLUE);
            }
        }
        // Display game message (whose turn, win/loss/draw)
        DrawText(message, 10, 10, 40, BLACK);
    } else {
        // If in difficulty selection state, call its specific drawing function
        drawDifficultySelection();
    }
}

// Function to reset the game state
void resetGame() {
    memset(board, EMPTY, sizeof(board));
    currentState = DIFFICULTY_SELECTION;
    gameOver = false;
    winner = EMPTY;
    strcpy(message, "Select Difficulty");
    // Difficulty is not reset here, keeps the last selected value.
}

// ----------------------- MAIN -----------------------

int main() {
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Connect Four - Raylib");
    SetTargetFPS(60);

    // Initialize game state (now uses global variables)
    resetGame(); // Initialize state using reset function

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        if (currentState == DIFFICULTY_SELECTION) {
            strcpy(message, "Select Difficulty"); // Keep message relevant
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePoint = GetMousePosition();
                Rectangle easyButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 30, 200, 50 };
                Rectangle mediumButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 30, 200, 50 };
                Rectangle hardButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 90, 200, 50 };

                if (CheckCollisionPointRec(mousePoint, easyButton)) {
                    difficulty = 2; // Easy
                    currentState = PLAYER_TURN;
                    strcpy(message, "Player's Turn (Click Column)");
                } else if (CheckCollisionPointRec(mousePoint, mediumButton)) {
                    difficulty = 4; // Medium
                    currentState = PLAYER_TURN;
                    strcpy(message, "Player's Turn (Click Column)");
                } else if (CheckCollisionPointRec(mousePoint, hardButton)) {
                    difficulty = 6; // Hard
                    currentState = PLAYER_TURN;
                    strcpy(message, "Player's Turn (Click Column)");
                }
            }
        } else if (!gameOver) { // Only process game turns if not selecting difficulty and game not over
            if (currentState == PLAYER_TURN) {
                 strcpy(message, "Player's Turn (Click Column)");
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int mouseX = GetMouseX();
                    // Check if click is within the board area horizontally and vertically
                    if (mouseX >= BOARD_OFFSET_X && mouseX < BOARD_OFFSET_X + COLS * CELL_SIZE &&
                        GetMouseY() >= BOARD_OFFSET_Y && GetMouseY() < BOARD_OFFSET_Y + ROWS * CELL_SIZE) {
                        int col = (mouseX - BOARD_OFFSET_X) / CELL_SIZE;

                        if (isValidMove2D(col)) {
                            makeMove2D(col, PLAYER);
                            if (winningMove2D(PLAYER)) {
                                gameOver = true;
                                winner = PLAYER;
                                strcpy(message, "You Win!");
                                currentState = GAME_OVER; // Explicitly set game over state
                            } else if (isFull2D()) {
                                gameOver = true;
                                winner = 3; // Draw
                                strcpy(message, "Draw!");
                                currentState = GAME_OVER; // Explicitly set game over state
                            } else {
                                currentState = AI_TURN;
                                // Message will be updated at the start of AI_TURN block
                            }
                        } else {
                             // Optionally provide feedback for invalid move click
                             // strcpy(message, "Invalid Move! Try again.");
                             // Maybe flash the column or play a sound
                        }
                    }
                }
            } else if (currentState == AI_TURN) {
                strcpy(message, "AI Thinking...");
                // AI's move (consider adding a small delay for visual effect if desired)
                // Draw frame before AI move calculation to show "Thinking..."
                BeginDrawing();
                ClearBackground(RAYWHITE);
                drawBoardRaylib(message, currentState); // Pass current state
                EndDrawing();

                int aiCol = getBestMove2D();
                 if (aiCol != -1) { // Ensure a valid move was found
                    makeMove2D(aiCol, AI);
                    if (winningMove2D(AI)) {
                        gameOver = true;
                        winner = AI;
                        strcpy(message, "AI Wins!");
                        currentState = GAME_OVER; // Explicitly set game over state
                    } else if (isFull2D()) {
                        gameOver = true;
                        winner = 3; // Draw
                        strcpy(message, "Draw!");
                        currentState = GAME_OVER; // Explicitly set game over state
                    } else {
                        currentState = PLAYER_TURN;
                        // Message will be updated at the start of PLAYER_TURN block
                    }
                 } else {
                     // This case should ideally not happen if isFull2D is checked correctly
                     // but as a fallback:
                     if (isFull2D()) {
                         gameOver = true;
                         winner = 3; // Draw
                         strcpy(message, "Draw! (AI found no moves)");
                         currentState = GAME_OVER; // Explicitly set game over state
                     } else {
                         // Handle unexpected error - maybe force player turn?
                         strcpy(message, "Error: AI failed to move!");
                         currentState = PLAYER_TURN;
                     }
                 }
            }
        } else { // Game is over (currentState == GAME_OVER)
             // Append restart instruction to the message only if not already present
             if (strstr(message, "Restart") == NULL) { // Check if restart text is already there
                 char finalMessage[150];
                 snprintf(finalMessage, sizeof(finalMessage), "%s Press 'R' to Restart.", message);
                 strcpy(message, finalMessage); // Update the message buffer
             }

             // Check for restart key press
             if (IsKeyPressed(KEY_R)) {
                 resetGame();
             }
        }


        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawBoardRaylib(message, currentState); // Draw based on current state

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();

    return 0;
}
