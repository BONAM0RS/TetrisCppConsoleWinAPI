
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

#include <Windows.h>


class Tetris {

public:
	Tetris();
	~Tetris();

private:
	void tickEvent();
	void onKeyPressed();

	void lockShape();
	void checkLines();
	void spawnShape();
	void addSpeed();
	void checkLosing();

	void drawField();
	void drawShape();
	void drawText();

	void destroyLines();

	bool checkCollision(int nTetromino, int nRotation, int nPosX, int nPosY);
	int rotate(int px, int py, int r);

private:
	HANDLE hConsole;
	DWORD dwBytesWritten;

	const int nFieldWidth = 12;
	const int nFieldHeight = 21;

	unsigned char* pField = nullptr;
	wchar_t* pScreen = nullptr;

	int nScreenWidth;  
	int nScreenHeight;

	int xOffset;
	int yOffset;

	wstring tetromino[7];

	bool bKey[5];

	bool bGameOver;
	bool bForceDown;
	bool bRotateHold;
	bool bPaused;
	bool bPauseHold;

	int nCurrentPiece;
	int nCurrentRotation;
	int nCurrentX;
	int nCurrentY;

	int nSpeed;
	int nSpeedCount;
	int nPieceCount;
	int nScore;
	int nLines;

	vector<int> vLines;
};