/* Cristian Cadalso & Kamren Freitag
Pong Game: Console-based Pong game for single player implemented in C++ for Debian/Ubuntu based Linux systems.
Features:
- Player controls a paddle at the bottom of the screen using 'A' (left) and 'D' (right) keys.
- A ball bounces around the screen, and the player must prevent it from falling past the paddle.
- A barrier is randomly placed on the screen, which the ball can bounce off of.
- The game ends when the ball falls past the paddle, and the player's score is displayed.
EE245: Robotics and Programming Algorithms, Spring 2026, MNSU
Date: 02/12/2026
*/

#include <iostream>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h> 
#include <fcntl.h> 
#include <time.h>

using namespace std;
struct termios orig_termios;

#define RESET   "\033[0m"
#define WALL    "\033[38;5;245m"   // soft gray
#define PADDLE  "\033[38;5;39m"    // bright cyan
#define BALL    "\033[38;5;82m"    // neon green
#define BARRIER "\033[38;5;214m"   // warm orange
#define SCORE   "\033[38;5;220m"   // gold

void clearScreen();
char readKey();
void enableRawMode();
void disableRawMode();

//Manages the Pong game state, rendering, and collision logic
class Pong {
    private:
        int rows, cols;
        string name;        
        int paddleX, velPaddle, score;
        int ballX, ballY, velX, velY;
        int barrierX, barrierY, barrierL;
        int sleepTime;

        //Best way to use wide characters in Linux is to use a String
        const string CHAR_WALL_H = "━";
        const string CHAR_WALL_V = "┃";
        const string CHAR_TL_CORNER = "┏";
        const string CHAR_TR_CORNER = "┓";
        const string CHAR_BL_CORNER = "┗";
        const string CHAR_BR_CORNER = "┛";
        const string CHAR_BALL = "●";
        const string CHAR_PADDLE = "▀";
        const string CHAR_BARRIER = "▒";

        void barrier() {
            barrierL = rand() % 5 + 2; // Barrier length between 2 and 6
             // Barrier spawns within the walls and 3-units from the edges
			barrierX = rand() % (cols - barrierL - 6) + 3; 
            // Barrier spawns within the walls and 3-units from the edges, and above the paddle
			barrierY = rand() % (rows - barrierL - 8) + 3; 
        }

    public:
        Pong(string name, int rows, int cols) {
            this->name = name;        
            this->rows = rows;
            this->cols = cols;
            score = 0;
            sleepTime = 300; // Controls the speed of the ball, greater there is, the slower
            paddleX = cols / 2;
            velPaddle = 2;
			ballX = rand() % (cols - 2) + 1; 
			ballY = rand() % (rows / 2) + 1;
            velX = 1;
            velY = 1;
            barrier();
            cout << "\033[?25l"; // Hide cursor
        }
       
        ~Pong() {
        cout << "\033[?25h" << RESET; // Show cursor on exit
        }

        void draw() {
            cout << "\033[H"; // Move cursor to home position (0,0)"

            string line1 = "Player: " + name;
            string line2 = "Score: " + to_string(score);
            int pad1 = (cols - line1.length()) / 2;
            int pad2 = (cols - line2.length()) / 2; 
            if (pad1 < 0) pad1 = 0;
            if (pad2 < 0) pad2 = 0;
            cout << string(pad1, ' ') << line1 << "\n" << string(pad2, ' ') << SCORE << line2 << RESET << "\n";

            for(int i = 0; i < rows; i++) {
                for(int j = 0; j < cols; j++) {
                    // Corners: top-left, top-right, bottom-left, bottom-right
                    if (i == 0 && j == 0)
                        cout << WALL << CHAR_TL_CORNER << RESET;
                    else if(i == 0 && j == cols - 1)
                        cout << WALL << CHAR_TR_CORNER << RESET;
                    else if(i == rows - 1 && j == 0)
                        cout << WALL << CHAR_BL_CORNER << RESET;
                    else if(i == rows - 1 && j == cols - 1)
                        cout << WALL << CHAR_BR_CORNER << RESET;
                    // Walls: if the row or column is at the border    
                    else if(i == 0 || i == rows - 1 )
                        cout << WALL << CHAR_WALL_H << RESET;
                    else if (j == 0 || j == cols - 1)
                        cout << WALL << CHAR_WALL_V << RESET;
                    // Paddle:
                    else if(i == rows - 2 && (j == paddleX || j == paddleX + 1))
                        cout << PADDLE << CHAR_PADDLE << RESET;
					// Ball
					else if(i == ballY && j == ballX) {
						cout << BALL << CHAR_BALL << RESET;
					}
					//Barrier
					else if (i >= barrierY && i < barrierY + barrierL && j >= barrierX && j < barrierX + barrierL){
						cout << BARRIER << CHAR_BARRIER << RESET;
					}
                    else
                        cout << " ";
                }
                cout << "\n";
            }
        }
    
        void handleInput() {
            char key = readKey();

            if (key == 'a' || key == 'A')
                paddleX -= velPaddle;
            else if (key == 'd' || key == 'D')
                paddleX += velPaddle;

            // Ensure paddle stays within the walls (1 unit from each wall)
            if (paddleX < 1) paddleX = 1;
            if (paddleX > cols - 3) paddleX = cols - 3;
}

    
    bool update() {
        int nextX = ballX + velX;
        int nextY = ballY + velY;
        
        // Wall bouncing
        if (nextX <= 0 || nextX >= cols - 1) velX = -velX;
        if (nextY <= 0) velY = -velY;

        // Barrier collision
        if (nextX >= barrierX && nextX < barrierX + barrierL &&
            nextY >= barrierY && nextY < barrierY + barrierL) {
                if(ballX < barrierX || ballX >= barrierX + barrierL) {
                    velX = -velX;
                } else {
                    velY = -velY;
                }
        }

        // Paddle collision: Checks if the ball's next position overlaps with the paddle coordinates
        if (nextY == rows - 2 && (nextX == paddleX || nextX == paddleX + 1)) {
            velY = -velY;
            score++;
            if (sleepTime > 30) sleepTime -= 5;
            barrier();
        }

        // Missed paddle
        if(nextY >= rows - 1) return false;

        ballX += velX;
        ballY += velY;
        return true;
    }

    int getSleepTime() {
        return sleepTime;
    }

    int getScore() {
        return score;
    }
};

int main() {
    srand(time(NULL));
    string playerName;
    int N, M;

    cout << "Enter Player Name: ";
    getline(cin, playerName);
    
    while(true) { 
        cout << "Input the Grid Size for the Barrier: ";
        cin >> N >> M;
        if (N == M or N < 20 or N > 40 or M < 20 or M >40) {
            cout << "Invalid input, try again. \n";
        } else {
            break;
        }
    } 

    Pong game(playerName, N, M); 

    clearScreen(); // To clear the console from previous outputs
    enableRawMode();

    while (true) {
        game.draw();
		game.handleInput();
        if (!game.update()) {
            cout << "Game Over! Final Score: " << game.getScore() << "\n";
            break;
        }
        usleep(game.getSleepTime() * 1000); // 
    
    }

    disableRawMode();
}

void clearScreen() {
    cout << "\033[2J\033[H"; // ANSI escape code to clear screen and move cursor to top-left
}

// Checks if a key is waiting in the buffer (Non-blocking)
char readKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1)
        return c;
    return 0;
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios); //Save original terminal settings

    termios raw = orig_termios;
    //Disable canonical mode(waits for Enter) and echoing(prints pressed keys to screen)
    raw.c_lflag &= ~(ICANON | ECHO);
    // Set read and read timeout to 0 for non-blocking input
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw); // Apply raw mod settings right now
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // Set keyboard to non-blocking mode
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    fcntl(STDIN_FILENO, F_SETFL, 0);
}

