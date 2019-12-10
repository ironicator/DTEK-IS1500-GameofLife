/* 
 Most of the code in this file is our own original code.
 Small parts of code have been taken from lab2, for example,
 some of the code in RenderToScreen() is taken from lab2.

 Latest update 2019-12-08 by Dilvan Sabir
 */

#include <math.h>
#include <pic32mx.h>
#include <stddef.h>
#include <stdint.h>
#include "byte_maps.h"

void u32init(void);

#define DISPLAY_CHANGE_TO_COMMAND_MODE (PORTFCLR = 0x10)
#define DISPLAY_CHANGE_TO_DATA_MODE (PORTFSET = 0x10)

#define WORLD_SIZE_X 100
#define WORLD_SIZE_Y 40

#define MID_X = WORLD_SIZE_X / 2
#define MID_Y = WORLD_SIZE_Y / 2

#define DARK_BACKGROUND 1

uint8_t world[WORLD_SIZE_X * WORLD_SIZE_Y];
uint8_t screenBuffer[32 * 128];

// Top left position of camera
int cameraX = WORLD_SIZE_X - 64;
int cameraY = WORLD_SIZE_Y - 16;

// Camera acceleration
double cameraAccX = 0;
double cameraAccY = 0;

// Button controls
char Btn1Down = 0;
char Btn2Down = 0;
char Btn3Down = 0;
char Btn4Down = 0;
char Btn34Down = 0;
char Btn23Down = 0;

// Switch controls
char switch1Down = 0;
char switch2Down = 0;
char switch3Down = 0;
char switch4Down = 0;

char pause = 0;
char reset;
char editMode;
char drawGen = 0;
char menuAlternative = 0;

unsigned int seed = 0;

unsigned int gameSpeed = 175;

void DrawPixel(int x, int y);
void DrawImage(const uint8_t *image);
void DrawPixelOnCamera(int x, int y);
void DrawCell(int x, int y);
void DrawWorld(uint8_t *world);
void NewCell(uint8_t *world, int x, int y);
char CellIsAlive(uint8_t *world, int x, int y);
void ClearWorld(uint8_t *world);
void NextGeneration(uint8_t *world);
void ClearScreen();
void RenderToScreen();
void DrawGens();
void DrawEditModeLabel();
void DrawPauseLabel();
int GetButtonInput();
int getSwitchInput();
int IsButtonPressed(int i);
int IsSwitchDown(int i);
void GameOfLifeLoop();
char MenuLoop();
void SettingsSubMenu();
char templateMenu = 0;
char settingsLoop = 0;
char fertility = 3;
int GenCounter = 0;

int main() {
    u32init();

    // Intro screen
    ClearScreen();
    DrawImage(intro_map);
    RenderToScreen();

    quicksleep(15000000);

    // Main game loop
    while (1) {
        reset = 0; // when set to 1, game exits to main loop

        menuAlternative = MenuLoop();
		
		// If the user has been in the template menu, continue the game
		if(templateMenu) {
			GameOfLifeLoop();
			
			continue;
		}
		
		ClearWorld(world);
		GenCounter = 0;
		
		if (menuAlternative == 0) { // First preset
			int x, y;

			for (y = 0; y < WORLD_SIZE_Y; y++) {
				for (x = 0; x < WORLD_SIZE_X; x++) {
					if (preset1[y * WORLD_SIZE_X + WORLD_SIZE_X]) {
						NewCell(world, x, y);
					}
				}
			}
			
			GameOfLifeLoop();
		}
		else if (menuAlternative == 1) { // Second preset
			// Glider
			NewCell(world, 0, 0);
			NewCell(world, 2, 0);
			NewCell(world, 2, 1);
			NewCell(world, 1, 1);
			NewCell(world, 1, 2);

			// Toad
			NewCell(world, 20, 10);
			NewCell(world, 21, 10);
			NewCell(world, 22, 10);
			NewCell(world, 19, 9);
			NewCell(world, 20, 9);
			NewCell(world, 21, 9);

			// Crazy thing
			NewCell(world, 40, 5);
			NewCell(world, 39, 6);
			NewCell(world, 40, 6);
			NewCell(world, 41, 6);
			NewCell(world, 38, 7);
			NewCell(world, 39, 7);
			NewCell(world, 40, 7);
			NewCell(world, 41, 7);
			NewCell(world, 42, 7);

			NewCell(world, 40, 16);
			NewCell(world, 39, 15);
			NewCell(world, 40, 15);
			NewCell(world, 41, 15);
			NewCell(world, 38, 14);
			NewCell(world, 39, 14);
			NewCell(world, 40, 14);
			NewCell(world, 41, 14);
			NewCell(world, 42, 14);
			
			GameOfLifeLoop();
		}
		else if (menuAlternative == 2) { // Random
			int x, y;

			for (y = 0; y < WORLD_SIZE_Y; y++) {
				for (x = 0; x < WORLD_SIZE_X; x++) {
					int life = (rand() % 10);
					if (life < fertility) NewCell(world, x, y);
				}
			}
			
			GameOfLifeLoop();
			
			
		}
		else if (menuAlternative == 3) {
			settingsLoop = 1;
			menuAlternative = 0;
		}

        // editMode = 0;

    }
}

//Random function from the C library (edited by us to include seed)
int rand(void) {
    unsigned int _next;

    _next = (_next * seed * 1103515245) + 12345;
	seed++;
    return ((_next >> 16) & 0x7fff);
}

void GameOfLifeLoop() {
	int n = 0;
	
	// Reset variables normally, unless returning from templateMenu
	if(!templateMenu) {
		pause = 0;
		editMode = 0;
	}
	else templateMenu = 0;

    // Loop for game of life
    while (1) {
        int i, j;

        // Poll buttons and switches every iteration

        // Place a cell when BTN4 is released
        /*if (IsButtonPressed(4) && IsButtonPressed(3)) {
            Btn34Down = 1;
		}
        else if (Btn34Down) {
            Btn34Down = 0;

            if (drawGen) drawGen = 0;
            else drawGen = 1;
        }*/
		
		// Place enter menu when BTN3 and BTN2 is pressed at the same time
		if (IsButtonPressed(3) && IsButtonPressed(2)) {
			// This is to block press in the new menu and wait for a button release
			Btn2Down = -1;
			Btn3Down = -1;
			
			templateMenu = 1;
			
			/* Here we temporarily break out of the game loop to
			   access the menu loop, but we will soon return */
			break;
		}

        // Place a cell when BTN4 is released
        if (IsButtonPressed(4)) {
            Btn4Down = 1;
		}
        else if (Btn4Down) {
            Btn4Down = 0;

            NewCell(world, cameraX / 2 + 32, cameraY / 2 + 8);
        }

        // Pause/unpause game when BTN3 is released
        if (IsButtonPressed(3)) {
            Btn3Down = 1;
		}
        else if (Btn3Down) {
            Btn3Down = 0;

            if (pause) pause = 0;
            else pause = 1;
        }

        // BTN 2 Enter edit mode
        // Enter edit mode when BTN2 is released
        if (IsButtonPressed(2)) Btn2Down = 1;
        else if (Btn2Down) {
            Btn2Down = 0;

            if (editMode) editMode = 0;
            else {
                editMode = 1;
                pause = 1; // Make sure the game is paused in edit mode
            }
        }

        // BTN1 Enter menu
        // Reset game when BTN1 is released
        if (IsButtonPressed(1)) Btn1Down = 1;
        else if (Btn1Down) {
            Btn1Down = 0;

            reset = 1;
            break;
        }

        // Camera logic in edit mode
        if (editMode) {
            if (IsSwitchDown(4)) switch4Down = 1;
            else if (switch4Down) {
                switch4Down = 0;
                cameraX -= 2;
            }

            if (IsSwitchDown(3)) switch3Down = 1;
            else if (switch3Down) {
                switch3Down = 0;
                cameraX += 2;
            }

            if (IsSwitchDown(2)) switch2Down = 1;
            else if (switch2Down) {
                switch2Down = 0;
                cameraY -= 2;
            }

            if (IsSwitchDown(1)) switch1Down = 1;
            else if (switch1Down) {
                switch1Down = 0;
                cameraY += 2;
            }
        }
		else { // Camera logic in normal mode
			// Accelerate camera
			if (IsSwitchDown(4)) cameraAccX -= 0.02;
			if (IsSwitchDown(3)) cameraAccX += 0.02;
			if (IsSwitchDown(2)) cameraAccY -= 0.02;
			if (IsSwitchDown(1)) cameraAccY += 0.02;
			
			// Poll switches in normal mode (when not in editing mode)
			if (n % 10 == 0) {
				cameraX += cameraAccX;
				cameraY += cameraAccY;

				// Don't let user scroll out of world
				if (cameraX + 128 > WORLD_SIZE_X * 2) {
					cameraAccX = 0;
					cameraX = WORLD_SIZE_X * 2 - 128;
				}
				if (cameraX < 0) {
					cameraAccX = 0;
					cameraX = 0;
				}
				if (cameraY + 32 > WORLD_SIZE_Y * 2) {
					cameraAccY = 0;
					cameraY = WORLD_SIZE_Y * 2 - 32;
				}
				if (cameraY < 0) {
					cameraAccY = 0;
					cameraY = 0;
				}
			}
		}
		
		if (n % 10 == 0) {
			ClearScreen();
			DrawWorld(world);
			
			//If we have generationlabel on, we will draw it
			if(drawGen) DrawGens();
			if(editMode) DrawEditModeLabel();
			if(pause) DrawPauseLabel();
			
			RenderToScreen();
		}
		

        // Deaccelerate camera
        if (cameraAccX < 0.005) cameraAccX += 0.01;
        else if (cameraAccX > 0.005) cameraAccX -= 0.01;

        if (cameraAccY < 0.005) cameraAccY += 0.01;
        else if (cameraAccY > 0.005) cameraAccY -= 0.01;

        if (cameraAccX > -0.01 && cameraAccX < 0.01) cameraAccX = 0;
        if (cameraAccY > -0.01 && cameraAccY < 0.01) cameraAccY = 0;

        if (pause == 0) {
            //continue;
            // New generation once every 100 iterations
			int adjustedGameSpeed = 206-gameSpeed;
            if (n % adjustedGameSpeed == 0) NextGeneration(world);
		}

        n++;
        if (reset) break;

        quicksleep(5000);
    }
}

/* Inserts an int into the char array settingsmenu
	arr = the row to insert it in.
	int p = the number to be inserted
	place = where to begin
*/
void int2char(int arr, int p, int place) {
    unsigned int c = place + count(p); // digit position

    int n = p;
	
	if(n == 0) settingsmenu[arr][c] = '0';

    // extract each digit
    while (n != 0) {
        settingsmenu[arr][c] = (n % 10) + '0';
        n /= 10;
        c--;
    }
}

char MenuLoop() {
    menuAlternative = 0;
	int menuAlternatives = 0;
	
    // Menu loop
    while (1) {
		seed++;
		
        ClearScreen();
        if (templateMenu == 1) {
			menuAlternatives = 3;
			
            int offset = (((128 / 8) - templatesMenuLength) / 2) * 8;

            int i, j, f;
            for (i = 0; i < 3; i++) {
                for (j = 0; j < templatesMenuLength; j++) {
                    for (f = 0; f < 8; f++) {
                        if (menuAlternative == i) {
                            screenBuffer[offset + i * 128 + j * 8 + f] = ~font[templates[i][j] * 8 + f];
                        }
                        else {
                            screenBuffer[offset + i * 128 + j * 8 + f] = font[templates[i][j] * 8 + f];
                        }
                    }
                }
            }
            RenderToScreen();

            // Choose current alternative
            if (IsButtonPressed(4)) Btn4Down = 1;
            else if (Btn4Down) {
                Btn4Down = 0;

                if (menuAlternative == 0) {
					int x = cameraX / 2 + 32;
					int y = cameraY / 2 + 8;
					
                    NewCell(world, x+0, y+0);
                    NewCell(world, x+2, y+0);
                    NewCell(world, x+2, y+1);
                    NewCell(world, x+1, y+1);
                    NewCell(world, x+1, y+2);
                }
                else if (menuAlternative == 1) {
					if(drawGen) drawGen = 0;
					else drawGen = 1;
                }
                else if (menuAlternative == 2) {
					ClearWorld(world);
                }

                break;
            }
        }
        else {
            //If we are in settings do this instead
            if (settingsLoop == 1) {
				menuAlternatives = 3;
				
				SettingsSubMenu();
			}
            else { // Main menu
				menuAlternatives = 4;
				
                int offset = (((128 / 8) - menuLength) / 2) * 8;
                int i, j, f;
                for (i = 0; i < 4; i++) {
                    for (j = 0; j < menuLength; j++) {
                        for (f = 0; f < 8; f++) {
                            if (menuAlternative == i && menu[i][j] != ' ') {
                                screenBuffer[offset + i * 128 + j * 8 + f] = ~font[menu[i][j] * 8 + f];
                            }
                            else {
                                screenBuffer[offset + i * 128 + j * 8 + f] = font[menu[i][j] * 8 + f];
                            }
                        }
                    }
                }
                RenderToScreen();
				
				// Choose current alternative
				if (IsButtonPressed(4)) Btn4Down = 1;
				else if (Btn4Down) {
					Btn4Down = 0;
					break;
				}
            }
        }

		// Go down one menu alternative
		if (IsButtonPressed(2) && Btn2Down != -1) Btn2Down = 1;
		else if (!IsButtonPressed(2) && Btn2Down == -1) Btn2Down = 0;
		else if (Btn2Down == 1) {
			menuAlternative = ++menuAlternative % menuAlternatives;
			Btn2Down = 0;
		}

		// Go up one menu alternative
		if (IsButtonPressed(3) && Btn3Down != -1) Btn3Down = 1;
		else if (!IsButtonPressed(3) && Btn3Down == -1) Btn3Down = 0;
		else if (Btn3Down == 1) {
			menuAlternative = --menuAlternative;
			if (menuAlternative == -1) menuAlternative = menuAlternatives-1;
			Btn3Down = 0;
		}
    }
    return menuAlternative;
}

// Calculates the next world generation
void NextGeneration(uint8_t *world) {
    uint8_t worldTemp[WORLD_SIZE_X * WORLD_SIZE_Y];
    ClearWorld(worldTemp);

    int x, y;

    for (y = 0; y < WORLD_SIZE_Y; y++) {
        for (x = 0; x < WORLD_SIZE_X; x++) {
            int neighbors = 0;
            int alive = CellIsAlive(world, x, y);

            // Count the amount of neighboring cells
            if (CellIsAlive(world, x - 1, y))
                neighbors++;
            if (CellIsAlive(world, x + 1, y))
                neighbors++;
            if (CellIsAlive(world, x, y - 1))
                neighbors++;
            if (CellIsAlive(world, x, y + 1))
                neighbors++;
            if (CellIsAlive(world, x - 1, y - 1))
                neighbors++;
            if (CellIsAlive(world, x - 1, y + 1))
                neighbors++;
            if (CellIsAlive(world, x + 1, y - 1))
                neighbors++;
            if (CellIsAlive(world, x + 1, y + 1))
                neighbors++;

            if (alive && neighbors >= 2 && neighbors <= 3) {
                NewCell(worldTemp, x, y);
			}
            else if (!alive && neighbors == 3) {
                NewCell(worldTemp, x, y);
			}
        }
    }

    int i;
    GenCounter++;
    // Write worldTemp to world
    for (i = 0; i < WORLD_SIZE_Y * WORLD_SIZE_X; i++) {
        world[i] = worldTemp[i];
    }
}

// Removes all cells from the world
void ClearWorld(uint8_t *world) {
    int i;

    for (i = 0; i < WORLD_SIZE_X * WORLD_SIZE_Y; i++) {
        world[i] = 0;
    }
}

// Spawn new cell
void NewCell(uint8_t *world, int x, int y) {
    int bitPlacement = 1 << (y % 8);

    world[y / 8 * WORLD_SIZE_X + x] |= bitPlacement;
}

// Returns 1 if cell is alive, 0 otherwise
char CellIsAlive(uint8_t *world, int x, int y) {
    // return 0 when checking a cell outside the world
    if (x < 0 || y < 0 || x >= WORLD_SIZE_X || y >= WORLD_SIZE_Y) return 0;

    int bitPlacement = 1 << (y % 8);

    return world[y / 8 * WORLD_SIZE_X + x] & bitPlacement;
}

/* Cells are 2x2 pixels and we change the grid system to
 account for that. This means that the position (1, 1)
 is actually located at pixels (2, 2), (3, 2), (3, 3)
 and (2, 3). */
void DrawCell(int x, int y) {
    DrawPixelOnCamera(x * 2, y * 2);
    DrawPixelOnCamera(x * 2 + 1, y * 2);
    DrawPixelOnCamera(x * 2, y * 2 + 1);
    DrawPixelOnCamera(x * 2 + 1, y * 2 + 1);
}

/* Draw all cells in world */
void DrawWorld(uint8_t *world) {
    int x, y;

    for (y = 0; y < WORLD_SIZE_Y; y++) {
        for (x = 0; x < WORLD_SIZE_X; x++) {
            if (CellIsAlive(world, x, y)) {
                DrawCell(x, y);
			}
        }
    }
}

/* x can be 0-127
 y can be 0-31 */
void DrawPixel(int x, int y) {
    // Don't draw pixels outside the screen area
    if (x < 0 || y < 0 || x >= 128 || y >= 32) return;

    int bitPlacement = 1 << (y % 8);

    screenBuffer[y / 8 * 128 + x] |= bitPlacement;
}

//Gives back the lenght of a int, how many decimals it contains
int count(int i) {
    int ret = 1;
    while (i /= 10) ret++;
	
    return ret;
}

void DrawGens() {
    int offset = 3 * 128;
    int arraysize = 4 + count(GenCounter);
    char sequence[arraysize];
    sequence[0] = 'G';
    sequence[1] = 'e';
    sequence[2] = 'n';
    sequence[3] = ':';

    int c = 3 + count(GenCounter); /* digit position */
    int n = GenCounter;

    /* extract each digit */
    while (n != 0) {
        sequence[c] = (n % 10) + '0';
        n /= 10;
        c--;
    }
	
	int ce;
    for (ce = 0; ce < sizeof(sequence); ce++) {
		int i;
        for (i = 0; i < 8; i++) {
            screenBuffer[offset + 0 + ce * 8 + i] = font[sequence[ce] * 8 + i];
        }
    }
}

void DrawEditModeLabel() {
	int offset = (((128 / 8) - 4)) * 8;
	
    char sequence[] = {'E', 'D', 'I', 'T'};
	
	int ce;
    for (ce = 0; ce < sizeof(sequence); ce++) {
		int i;
        for (i = 0; i < 8; i++) {
            screenBuffer[offset + 3*128 + ce * 8 + i] = font[sequence[ce] * 8 + i];
        }
    }
}

void DrawPauseLabel() {
	int offset = (((128 / 8) - 5)) * 8;
	
    char sequence[] = {'P', 'A', 'U', 'S', 'E'};
	
	int ce;
    for (ce = 0; ce < sizeof(sequence); ce++) {
		int i;
        for (i = 0; i < 8; i++) {
            screenBuffer[offset + ce * 8 + i] = font[sequence[ce] * 8 + i];
        }
    }
}

// Draw a pixel adjusted to where the camera is
void DrawPixelOnCamera(int x, int y) {
    // Adjust where pixels are drawn according to camera
    x -= cameraX;
    y -= cameraY;

    DrawPixel(x, y);
}

// Show intro image
void DrawImage(const uint8_t *image) {
    int x, y;

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 128; x++) {
            if (image[128 * y + x] < 0x99) {
                DrawPixel(x, y);
			}
        }
    }
}

// Renders to screen
void RenderToScreen() {
    int i, j;

    for (i = 0; i < 4; i++) {
        DISPLAY_CHANGE_TO_COMMAND_MODE;

        spi_send_recv(0x22);
        spi_send_recv(i);

        spi_send_recv(0);
        spi_send_recv(0x10);

        DISPLAY_CHANGE_TO_DATA_MODE;

        for (j = 0; j < 128; j++) {
            if (DARK_BACKGROUND) {
				spi_send_recv(screenBuffer[i * 128 + j]);
			}
            else {
                spi_send_recv(~screenBuffer[i * 128 + j]);
			}
        }
    }
}

/* Clear the screen */
void ClearScreen() {
    int i;

    for (i = 0; i < 512; i++) {
        screenBuffer[i] = 0;
    }
}

// Returns the button bits for buttons 2-4
int GetButtonInput() {
    return (PORTD & (0x7 << 5)) >> 4;
}

// Returns the switch bits for switches 1-4
int getSwitchInput() {
    return (PORTD & (0xF << 8)) >> 8;
}

// Returns 1 if button i is pressed, 0 otherwise
int IsButtonPressed(int i) {
    if (PORTF & 0x2 && i == 1) {
        return 1; // Button 1
	}

    int buttonValue = GetButtonInput();

    return (buttonValue >> (i - 1)) & 0x1;
}

// Returns 1 if switch i is down, 0 otherwise
int IsSwitchDown(int i) {
    int switchValue = getSwitchInput();

    return (switchValue >> (i - 1)) & 0x1;
}

void SettingsSubMenu() {
	if (IsSwitchDown(2)) switch2Down = 1;
	else if (switch2Down) {
		switch2Down = 0;
		if (menuAlternative == 0) {
			if (gameSpeed >= 200) {
				gameSpeed = 200;
			}
			else {
				gameSpeed += 5;
			}
		}

		if (menuAlternative == 1) {
			if (drawGen == 0) {
				drawGen = 1;
			}
			else {
				drawGen = 0;
			}
		}

		if (menuAlternative == 2) {
			if (fertility >= 9) {
				fertility = 9;
			}
			else {
				fertility += 1;
			}
		}
	}

	if (IsSwitchDown(1)) switch1Down = 1;
	else if (switch1Down) {
		switch1Down = 0;
		
		if (menuAlternative == 0) {
			if (gameSpeed <= 5) {
				gameSpeed = 5;
			}
			else {
				gameSpeed -= 5;
			}
		}

		if (menuAlternative == 1) {
			if (drawGen == 0) {
				drawGen = 1;
			}
			else {
				drawGen = 0;
			}
		}

		if (menuAlternative == 2) {
			if (fertility <= 1) {
				fertility = 1;
			}
			else {
				fertility -= 1;
			}
		}
	}

	// Clear speed number
	settingsmenu[0][7] = ' ';
	settingsmenu[0][8] = ' ';
	settingsmenu[0][9] = ' ';
	
	//Inserts these variables into the char array so it can be displayed
	int2char(0, gameSpeed, 6);
	int2char(1, drawGen, 9); //this one does not work for some reason
	int2char(2, fertility, 9);

	int offset = (((128 / 8) - settingsmenuLength) / 2) * 8;

	int i, j, f;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < settingsmenuLength; j++) {
			for (f = 0; f < 8; f++) {
				if (menuAlternative == i) {
					//On the current menu option, inverse colors to mark it
					screenBuffer[offset + i * 128 + j * 8 + f] = ~font[settingsmenu[i][j] * 8 + f];
				}
				else {
					//On the non-current menu option, normal colors
					screenBuffer[offset + i * 128 + j * 8 + f] = font[settingsmenu[i][j] * 8 + f];
				}
			}
		}
	}

	if (IsButtonPressed(4)) Btn4Down = 1;
	else if (Btn4Down) {
		Btn4Down = 0;
		settingsLoop = 0;
		menuAlternative = 3; // After exiting settings menu, keep selection of settings button
	}

	RenderToScreen();
}
