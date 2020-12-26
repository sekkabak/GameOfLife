#include <Windows.h>
#include <iostream>
#include <thread>

using namespace std;

class GameOfLife
{
	HANDLE consoleOutputHandle;
	HANDLE consoleInputHandle;
	CHAR_INFO* screenBuffer;
	SMALL_RECT rectWindow;
	int screenWidth = 80;
	int screenHeight = 80;
	bool new_map[80][80];
	bool map[80][80];
	int num_of_cells = 80;

	short m_keyOldState[256] = { 0 };
	short m_keyNewState[256] = { 0 };

	double symulation_speed = 1.0;
	double symulation_speed_step = .1;
	bool symulation_playing = false;

	struct sKeyState
	{
		bool bPressed;
		bool bReleased;
		bool bHeld;
	} m_keys[256];

	void SetCell(bool state, int x, int y)
	{
		map[x][y] = state;
		Draw(x, y, ' ', 0xff);
	}

	int GetLivingNeighbourCellsCount(int x, int y)
	{
		int sum = 0;

		// górna linia
		if (y > 0 && x > 0 && map[x - 1][y - 1]) sum++;
		if (y > 0 && map[x][y - 1]) sum++;
		if (y > 0 && x < screenWidth - 1 && map[x + 1][y - 1]) sum++;

		// srodkowa linia
		if (x > 0 && map[x - 1][y]) sum++;
		if (x < screenWidth - 1 && map[x + 1][y]) sum++;

		// dolna linia
		if (y < screenHeight - 1 && x > 0 && map[x - 1][y + 1]) sum++;
		if (y < screenHeight - 1 && map[x][y + 1]) sum++;
		if (y < screenHeight - 1 && x < screenWidth - 1 && map[x + 1][y + 1]) sum++;

		return sum;
	}

	void MakeStep()
	{
		for (int x = 0; x < num_of_cells; x++) {
			for (int y = 0; y < num_of_cells; y++) {
				new_map[x][y] = false;
			}
		}
		
		for (int x = 0; x < num_of_cells; x++) {
			for (int y = 0; y < num_of_cells; y++) {
				const int livingNeighbours = GetLivingNeighbourCellsCount(x, y);

				if (map[x][y] == true) {
					// overpopulation
					if (livingNeighbours >= 4) {
						new_map[x][y] = false;
					}
					// survives
					else if (livingNeighbours >= 2) {
						// rewrite
						new_map[x][y] = map[x][y];
					}
					// solitude
					else if (livingNeighbours <= 1) {
						new_map[x][y] = false;
					}
				} else {
					// birth
					if (livingNeighbours == 3) {
						new_map[x][y] = true;
					}
				}
			}
		}

		for (int x = 0; x < num_of_cells; x++) {
			for (int y = 0; y < num_of_cells; y++) {
				map[x][y] = new_map[x][y];
			}
		}
	}

	void InitConsole()
	{
		consoleOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		consoleInputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		SMALL_RECT rect = {0, 0, 1, 1};
		SetConsoleWindowInfo(consoleOutputHandle, TRUE, &rect);


		COORD coord = {static_cast<short>(screenWidth), static_cast<short>(screenHeight)};
		SetConsoleScreenBufferSize(consoleOutputHandle, coord);
		SetConsoleActiveScreenBuffer(consoleOutputHandle);

		screenBuffer = new CHAR_INFO[screenWidth * screenHeight];
		memset(screenBuffer, 0, sizeof(CHAR_INFO) * screenWidth * screenHeight);

		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof cfi;
		cfi.nFont = 0;
		cfi.dwFontSize.X = 8;
		cfi.dwFontSize.Y = 8;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(consoleOutputHandle, false, &cfi);
		rectWindow = {0, 0, static_cast<short>(screenWidth) - 1, static_cast<short>(screenHeight) - 1};
		SetConsoleWindowInfo(consoleOutputHandle, TRUE, &rectWindow);
		SetConsoleMode(consoleInputHandle, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
	}

	void Draw(const int x, const int y, const short c, const short color = 0x000F)
	{
		if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight) {
			int index = y * screenWidth + x;
			screenBuffer[index].Char.UnicodeChar = c;
			screenBuffer[index].Attributes = color;
		}
	}

	void ReDrawMap()
	{
		for (int y = 0; y < num_of_cells; y++) {
			for (int x = 0; x < num_of_cells; x++) {
				if (map[x][y])
					Draw(x, y, ' ', 0xff);
				else
					Draw(x, y, ' ');
			}
		}
	}

	void HandleInput()
	{
		// Handle Keyboard Input
		for (int i = 0; i < 256; i++) {
			m_keyNewState[i] = GetAsyncKeyState(i);

			m_keys[i].bPressed = false;
			m_keys[i].bReleased = false;

			if (m_keyNewState[i] != m_keyOldState[i])
			{
				if (m_keyNewState[i] & 0x8000)
				{
					m_keys[i].bPressed = !m_keys[i].bHeld;
					m_keys[i].bHeld = true;
				}
				else
				{
					m_keys[i].bReleased = true;
					m_keys[i].bHeld = false;
				}
			}

			m_keyOldState[i] = m_keyNewState[i];
		}
	}

	void GameThread()
	{
		OnStart();
		GameLoop();
	}

	double sim_time = 0.0;
	void GameLoop()
	{
		while (true)
		{
			HandleInput();
			if (this->symulation_playing) {
				if (sim_time >= this->symulation_speed) {
					MakeStep();
					sim_time = 0.0;
				} else {
					Sleep(10 * symulation_speed_step);
					sim_time += symulation_speed_step;
				}
			}
		
			OnUpdate();
			WriteConsoleOutput(consoleOutputHandle, screenBuffer,
				{
					static_cast<short>(screenWidth),
					static_cast<short>(screenHeight)
				},
				{ 0, 0 },
				&rectWindow);
		}
	}

	void OnStart()
	{
		// square
		// SetCell(true, 10, 70);
		// SetCell(true, 11, 70);
		// SetCell(true, 12, 70);	
		// SetCell(true, 10, 71);
		// SetCell(true, 12, 71);
		// SetCell(true, 10, 72);
		// SetCell(true, 11, 72);
		// SetCell(true, 12, 72);

		// flying sth
		// SetCell(true, 6, 3);
		// SetCell(true, 7, 4);
		// SetCell(true, 5, 5);
		// SetCell(true, 6, 5);
		// SetCell(true, 7, 5);

		// shooting sth
		SetCell(true, 34, 6);
		SetCell(true, 34, 7);
		SetCell(true, 35, 6);
		SetCell(true, 35, 7);

		SetCell(true, 39, 6);
		SetCell(true, 40, 5);
		SetCell(true, 41, 4);
		SetCell(true, 40, 7);
		SetCell(true, 41, 8);
		SetCell(true, 42, 5);
		SetCell(true, 42, 6);
		SetCell(true, 42, 7);
		SetCell(true, 42, 5);
		SetCell(true, 43, 3);
		SetCell(true, 43, 4);
		SetCell(true, 43, 8);
		SetCell(true, 43, 9);

		SetCell(true, 57, 8);
		SetCell(true, 58, 7);
		SetCell(true, 58, 8);
		SetCell(true, 58, 9);
		SetCell(true, 59, 6);
		SetCell(true, 59, 7);
		SetCell(true, 59, 8);
		SetCell(true, 59, 9);
		SetCell(true, 59, 10);
		SetCell(true, 60, 5);
		SetCell(true, 60, 6);
		SetCell(true, 60, 10);
		SetCell(true, 60, 11);

		SetCell(true, 64, 7);
		SetCell(true, 64, 8);
		SetCell(true, 64, 9);
		SetCell(true, 65, 7);
		SetCell(true, 65, 8);
		SetCell(true, 65, 9);

		SetCell(true, 68, 8);
		SetCell(true, 68, 9);
		SetCell(true, 69, 8);
		SetCell(true, 69, 9);
	}

	void OnUpdate()
	{
		// arrow up
		if (m_keys[38].bReleased)
		{
			// symulation speed up
			symulation_speed -= symulation_speed_step;
			if (this->symulation_speed <= 0)
				this->symulation_speed = 0;
		}

		// arrow down
		if (m_keys[40].bReleased)
		{
			// symulation speed down
			symulation_speed += symulation_speed_step;
		}
		
		// arrow right
		if (m_keys[39].bReleased)
		{
			// dalszy krok symulacji
			MakeStep();
		}

		// arrow left
		if (m_keys[37].bReleased)
		{
			symulation_speed = 1.0;
			symulation_playing = !symulation_playing;
		}

		ReDrawMap();
	}

public:
	GameOfLife()
	{
		InitConsole();
		auto t = std::thread(&GameOfLife::GameThread, this);
		t.join();
	}
};

int main()
{
	GameOfLife gof;

	return 0;
}
