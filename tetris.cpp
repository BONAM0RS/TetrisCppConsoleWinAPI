
#include "tetris.h"


Tetris::Tetris()
{
	hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	dwBytesWritten = 0;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	nScreenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;  // console columns
	nScreenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1; // comsole rows

	xOffset = nScreenWidth / 2 - nFieldWidth - 4;
	yOffset = (nScreenHeight - nFieldHeight) / 2;

	CONSOLE_FONT_INFOEX font;
	font.cbSize = sizeof font;
	font.dwFontSize.X = 0;
	font.dwFontSize.Y = 24;
	wcscpy_s(font.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(hConsole, false, &font);

	// Create screen buffer
	pScreen = new wchar_t[nScreenWidth * nScreenHeight];
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
	{
		pScreen[i] = L' ';
	}

	// Create field buffer
	pField = new unsigned char[nFieldWidth * nFieldHeight]; 
	for (int x = 0; x < nFieldWidth; x++) 
	{
		for (int y = 0; y < nFieldHeight; y++)
		{
			pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;
		}
	}
		
	// Tetronimos 4x4
	tetromino[0].append(L"..X...X...X...X."); // I
	tetromino[1].append(L"..X..XX...X....."); // T
	tetromino[2].append(L".....XX..XX....."); // O
	tetromino[3].append(L"..X..XX..X......"); // Z
	tetromino[4].append(L".X...XX...X....."); // S
	tetromino[5].append(L".X...X...XX....."); // L
	tetromino[6].append(L"..X...X..XX....."); // J

	// Create seed and do first spawn
	srand((unsigned int)time(0));
	spawnShape();

	// Variables
	nSpeed = 20;
	nSpeedCount = 0;

	nPieceCount = 0;
	nScore = 0;
	nLines = 0;

	// Bools
	bGameOver = false;
	bForceDown = false;
	bRotateHold = false;
	bPaused = false;
	bPauseHold = false;

	// Start game loop
	tickEvent();
}


Tetris::~Tetris()
{
	delete[] pScreen;
	delete[] pField;
}


void Tetris::tickEvent()
{
	// Main Loop
	while (!bGameOver) 
	{
		// Game tick delay
		this_thread::sleep_for(50ms);

		// Input processing
		onKeyPressed();

		if (bPaused == false)
		{
			nSpeedCount++;

			// 50ms delay * 20 speed = 1000 ms, so it's 1 move per sec 
			if (nSpeedCount == nSpeed) { bForceDown =  true; } 
			else                       { bForceDown = false; }

			// Force the piece down the playfield if it's time
			if (bForceDown)
			{
				nSpeedCount = 0;

				// Test if piece can be moved down
				if (checkCollision(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
				{
					nCurrentY++;
				}
				else
				{
					lockShape();
					checkLines();
					spawnShape();
					addSpeed();

					// If piece have collision on respawn, game over!
					checkLosing();
				}
			}
		}
		
		// Draw objects
		drawField();
		drawShape();
		drawText();

		// Replace full row with next above
		destroyLines();

		// Update Frame
		WriteConsoleOutputCharacter(hConsole, pScreen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}
}


void Tetris::onKeyPressed()
{
	// Input ========================
	for (int k = 0; k < 5; k++)
	{
		bKey[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x25\x27\x28\x26P"[k]))) != 0; // Left, Right, Down, Up, P
	}
		
	// Game Logic ===================
	if (bPaused == false)
	{
		// Move left, right, down
		nCurrentX -= (bKey[0] && checkCollision(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0;
		nCurrentX += (bKey[1] && checkCollision(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0;
		nCurrentY += (bKey[2] && checkCollision(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) ? 1 : 0;

		// Rotation
		if (bKey[3])
		{
			nCurrentRotation += (!bRotateHold && checkCollision(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
			bRotateHold = true;
		}
		else
		{
			bRotateHold = false;
		}	
	}

	// Pause
	if (bKey[4])
	{
		if (!bPaused && !bPauseHold)
		{
			bPaused = true;
		}

		else if (bPaused && !bPauseHold)
		{
			bPaused = false;
		}

		bPauseHold = true;

	}
	else
	{
		bPauseHold = false;
	}
}


void Tetris::lockShape()
{
	for (int px = 0; px < 4; px++)
	{
		for (int py = 0; py < 4; py++)
		{
			if (tetromino[nCurrentPiece][rotate(px, py, nCurrentRotation)] != L'.')
			{
				pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;
			}	
		}	
	}	
}


void Tetris::checkLines()
{
	for (int py = 0; py < 4; py++)
	{
		if (nCurrentY + py < nFieldHeight - 1)
		{
			bool bLine = true;
			for (int px = 1; px < nFieldWidth - 1; px++)
			{
				bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;
			}
				
			if (bLine)
			{
				// Remove Line, set to =
				for (int px = 1; px < nFieldWidth - 1; px++)
				{
					pField[(nCurrentY + py) * nFieldWidth + px] = 8;
				}
					
				vLines.push_back(nCurrentY + py);

				nLines++;
			}
		}
	}
		
	nScore += 25;

	if (!vLines.empty())
	{
		nScore += (1 << vLines.size()) * 100;
	}
}


void Tetris::spawnShape()
{
	nCurrentX = nFieldWidth / 2;
	nCurrentY = 0;
	nCurrentRotation = 0;
	nCurrentPiece = rand() % 7;
	nPieceCount++;
}


void Tetris::addSpeed()
{
	// Update difficulty every 10 pieces
	if (nPieceCount % 10 == 0)
	{
		if (nSpeed > 11)
		{
			nSpeed--;
		}
	}	
}


void Tetris::checkLosing()
{
	bGameOver = !checkCollision(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);

	if (bGameOver)
	{
		CloseHandle(hConsole);
		cout << "Game Over! Score:" << nScore << endl;
		system("pause");
	}
}


void Tetris::destroyLines()
{
	// Animate Line Completion
	if (!vLines.empty())
	{
		// Display Frame (cheekily to draw lines)
		WriteConsoleOutputCharacter(hConsole, pScreen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
		this_thread::sleep_for(400ms); // Delay a bit

		for (auto& v : vLines)
		{
			for (int px = 1; px < nFieldWidth - 1; px++)
			{
				for (int py = v; py > 0; py--)
				{
					pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
				}
					
				pField[px] = 0;
			}
		}
			
		vLines.clear();
	}
}


void Tetris::drawField()
{
	for (int x = 0; x < nFieldWidth; x++)
	{
		for (int y = 0; y < nFieldHeight; y++)
		{
			pScreen[(y + yOffset) * nScreenWidth + (x + xOffset)] = L" ABCDEFG=#"[pField[y * nFieldWidth + x]];
		}
	}		
}


void Tetris::drawShape()
{
	for (int px = 0; px < 4; px++)
	{
		for (int py = 0; py < 4; py++)
		{
			if (tetromino[nCurrentPiece][rotate(px, py, nCurrentRotation)] != L'.')
			{
				pScreen[(nCurrentY + py + yOffset) * nScreenWidth + (nCurrentX + px + xOffset)] = nCurrentPiece + 65;
			}
		}
	}			
}


void Tetris::drawText()
{
	swprintf_s(&pScreen[(yOffset + 0) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"===============");
	swprintf_s(&pScreen[(yOffset + 1) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"KEY CONTROLS:  ");
	swprintf_s(&pScreen[(yOffset + 2) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Rotate        ^");
	swprintf_s(&pScreen[(yOffset + 3) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Move left     <");
	swprintf_s(&pScreen[(yOffset + 4) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Move right    >");
	swprintf_s(&pScreen[(yOffset + 5) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Move down     v");
	swprintf_s(&pScreen[(yOffset + 6) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Pause         P");

	swprintf_s(&pScreen[(yOffset + 8) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"INFO TABLE:    ");
	swprintf_s(&pScreen[(yOffset + 9) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Speed  %8d", 20 - nSpeed + 1);
	swprintf_s(&pScreen[(yOffset + 10) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Lines  %8d", nLines);
	swprintf_s(&pScreen[(yOffset + 11) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Pieces %8d", nPieceCount);
	swprintf_s(&pScreen[(yOffset + 12) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"Score  %8d", nScore);
	swprintf_s(&pScreen[(yOffset + 13) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"===============");

	if (bPaused)
	{
		swprintf_s(&pScreen[(yOffset + 16) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"===============");
		swprintf_s(&pScreen[(yOffset + 18) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"  GAME PAUSED  ");
		swprintf_s(&pScreen[(yOffset + 20) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"===============");
	}
	else
	{
		swprintf_s(&pScreen[(yOffset + 16) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"               ");
		swprintf_s(&pScreen[(yOffset + 18) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"               ");
		swprintf_s(&pScreen[(yOffset + 20) * nScreenWidth + nFieldWidth + xOffset + 6], 16, L"               ");
	}
}


bool Tetris::checkCollision(int nTetromino, int nRotation, int nPosX, int nPosY)
{
	for (int px = 0; px < 4; px++)
	{
		for (int py = 0; py < 4; py++)
		{
			// Get index into piece
			int pi = rotate(px, py, nRotation);

			// Get index into field
			int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

			if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
			{
				if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
				{
					// In Bounds so do collision check
					if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}


int Tetris::rotate(int px, int py, int r)
{
	int pi = 0;
	switch (r % 4)
	{
		//  0 degrees
		/*
		//  0  1  2  3
		//  4  5  6  7
		//  8  9 10 11
		// 12 13 14 15
		*/
		case 0: { pi = py * 4 + px;        break; }
		                            
		// 90 degrees
		/*
		// 12  8  4  0
		// 13  9  5  1
		// 14 10  6  2
		// 15 11  7  3
		*/
		case 1: { pi = 12 + py - (px * 4); break; }		
			
		// 180 degrees
		/*
		// 15 14 13 12
		// 11 10  9  8
		//  7  6  5  4
		//  3  2  1  0
		*/
		case 2: { pi = 15 - (py * 4) - px; break; }
		
		// 270 degrees
		/*
		//  3  7 11 15
		//  2  6 10 14
		//  1  5  9 13
		//  0  4  8 12
		*/
		case 3: { pi = 3 - py + (px * 4);  break; }
	}								

	return pi;
}

