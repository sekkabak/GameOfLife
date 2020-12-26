#include <Windows.h>
#include <iostream>
#include <thread>

// left arrow start/stop simulation
// top/down arrow speed/slow simulation speed
// right arrow single step in simulation
class GameOfLife
{
	HANDLE consoleOutputHandle;
	HANDLE consoleInputHandle;
	CHAR_INFO* screenBuffer;
	SMALL_RECT rectWindow;
	int screenWidth = 80;
	int screenHeight = 80;
	bool new_map_[80][80];
	bool map_[80][80];

	double simulation_speed = 1.0;
	double simulation_speed_step = .05;
	bool simulation_playing = false;

	void SetCell(const bool state, const int x, const int y)
	{
		map_[x][y] = state;
	}

	int GetLivingNeighbourCellsCount(const int x, const int y)
	{
		int sum = 0;

		// top line
		if (y > 0 && x > 0 && map_[x - 1][y - 1]) sum++;
		if (y > 0 && map_[x][y - 1]) sum++;
		if (y > 0 && x < screenWidth - 1 && map_[x + 1][y - 1]) sum++;

		// middle line
		if (x > 0 && map_[x - 1][y]) sum++;
		if (x < screenWidth - 1 && map_[x + 1][y]) sum++;

		// bottom line
		if (y < screenHeight - 1 && x > 0 && map_[x - 1][y + 1]) sum++;
		if (y < screenHeight - 1 && map_[x][y + 1]) sum++;
		if (y < screenHeight - 1 && x < screenWidth - 1 && map_[x + 1][y + 1]) sum++;

		return sum;
	}

	void ClearNewMap()
	{
		for (int x = 0; x < screenWidth; x++)
			for (int y = 0; y < screenHeight; y++)
				new_map_[x][y] = false;
	}

	void SaveNewMapToActualMap()
	{
		for (int x = 0; x < screenWidth; x++)
			for (int y = 0; y < screenHeight; y++)
				map_[x][y] = new_map_[x][y];
	}

	// Calculates if given cell should die or spawn
	void HandleCellInUnitOfTime(const int x, const int y)
	{
		const int living_neighbours = GetLivingNeighbourCellsCount(x, y);

		if (map_[x][y] == true)
		{
			// dies bcs of overpopulation
			if (living_neighbours >= 4)
				new_map_[x][y] = false;
				// survives
			else if (living_neighbours >= 2)
				new_map_[x][y] = map_[x][y]; // rewrite
				// dies bcs of solitude
			else if (living_neighbours <= 1)
				new_map_[x][y] = false;
		}
		else
		{
			// birth of new cell
			if (living_neighbours == 3)
			{
				new_map_[x][y] = true;
			}
		}
	}

	// Step in simulation means that one unit of time for all cells has passed
	void MakeStep()
	{
		ClearNewMap();

		// Handle all cell changes in one unit of time
		for (int x = 0; x < screenWidth; x++)
			for (int y = 0; y < screenHeight; y++)
				HandleCellInUnitOfTime(x, y);

		SaveNewMapToActualMap();
	}

	// Lot of things to set console for proper displaing
	// Most of things are from olcConsoleGameEngine by javidx9
	// Check him out https://www.youtube.com/c/javidx9 :)
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
		rectWindow = {0, 0, static_cast<short>(screenWidth - 1), static_cast<short>(screenHeight - 1)};
		SetConsoleWindowInfo(consoleOutputHandle, TRUE, &rectWindow);
		SetConsoleMode(consoleInputHandle, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
	}

	void Draw(const int x, const int y, const short c, const short color = 0x000F)
	{
		if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight)
		{
			const int index = y * screenWidth + x;
			screenBuffer[index].Char.UnicodeChar = c;
			screenBuffer[index].Attributes = color;
		}
	}

	void ReDrawMap()
	{
		for (int x = 0; x < screenWidth; x++)
		{
			for (int y = 0; y < screenHeight; y++)
			{
				if (map_[x][y])
					Draw(x, y, ' ', 0xff); // this makes all pixels in char space to be white(dunno why but its cool) 
				else
					Draw(x, y, ' ');
			}
		}
	}

	short actual_key_state_[4];
	short old_key_state_[4];

	struct KeyState
	{
		bool pressed;
		bool released;
	} arrow_keys_[4];

#define ARROW_LEFT_INDEX 0
#define ARROW_UP_INDEX 1
#define ARROW_RIGHT_INDEX 2
#define ARROW_DOWN_INDEX 3

	// Reads key state which is saved in actual_key_state_
	// Also compares new key state to old key state for key release
	// Reads only arrow keys
	void HandleInput()
	{
		// Keycodes:
		// 37 - arrow left
		// 38 - arrow up
		// 39 - arrow right
		// 40 - arrow down

		int key_map[4] = {37, 38, 39, 40};
		for (int i = 0; i < 4; i++)
		{
			// My simulation is running on other thread so I use async
			actual_key_state_[i] = GetAsyncKeyState(key_map[i]);
			arrow_keys_[i].pressed = false;
			arrow_keys_[i].released = false;

			// Avoid multiple onPress
			if (actual_key_state_[i] != old_key_state_[i])
			{
				// GetAsyncKeyState returns 2 bytes number
				// If the most significant bit is set, the key is down
				if (actual_key_state_[i] >> 15 == 0x1)
				{
					arrow_keys_[i].pressed = true;
				}
				else
				{
					arrow_keys_[i].released = true;
				}
			}

			old_key_state_[i] = actual_key_state_[i];
		}
	}

	void UpdateConsoleTitle()
	{
		wchar_t s[256];
		swprintf_s(s, 256, L"Game Of Life - Simulation Speed: %3.2f", 1.0 - simulation_speed);
		SetConsoleTitle(s);
	}

	void ModifySimulationSpeed(double mod_number)
	{
		simulation_speed += mod_number;
		if (this->simulation_speed <= 0)
			this->simulation_speed = 0;

		UpdateConsoleTitle();
	}

	void GameThread()
	{
		UpdateConsoleTitle();
		OnStart();
		GameLoop();
	}

	double sim_time = 0.0;

	void GameLoop()
	{
		while (true)
		{
			HandleInput();
			if (this->simulation_playing)
			{
				if (sim_time >= this->simulation_speed)
				{
					MakeStep();
					sim_time = 0.0;
				}
				else
				{
					Sleep(10 * simulation_speed_step);
					sim_time += simulation_speed_step;
				}
			}

			OnUpdate();
			WriteConsoleOutput(consoleOutputHandle, screenBuffer,
			                   {
				                   static_cast<short>(screenWidth),
				                   static_cast<short>(screenHeight)
			                   },
			                   {0, 0},
			                   &rectWindow);
		}
	}

	void OnUpdate()
	{
		// simulation on/off
		if (arrow_keys_[ARROW_LEFT_INDEX].released) simulation_playing = !simulation_playing;

		// simulation speed up
		if (arrow_keys_[ARROW_UP_INDEX].released) ModifySimulationSpeed(-simulation_speed_step);

		// simulation speed down
		if (arrow_keys_[ARROW_DOWN_INDEX].released) ModifySimulationSpeed(simulation_speed_step);

		// simulation step forward
		if (arrow_keys_[ARROW_RIGHT_INDEX].released) MakeStep();

		ReDrawMap();
	}

protected:

	// Here I can fill map with living cells for simulation to actuall work
	void OnStart()
	{
		// shooting sth
		DrawShootingSth(0, 0);
		DrawShootingSth(20, 20);
		DrawShootingSth(40, 40);
	}

public:

	// Draws shooting entity on map
	// Starts on given x and y
	void DrawShootingSth(const int x, const int y)
	{
		SetCell(true, x + 4, y + 6);
		SetCell(true, x + 4, y + 7);
		SetCell(true, x + 5, y + 6);
		SetCell(true, x + 5, y + 7);

		SetCell(true, x + 9, y + 6);
		SetCell(true, x + 10, y + 5);
		SetCell(true, x + 11, y + 4);
		SetCell(true, x + 10, y + 7);
		SetCell(true, x + 11, y + 8);
		SetCell(true, x + 12, y + 5);
		SetCell(true, x + 12, y + 6);
		SetCell(true, x + 12, y + 7);
		SetCell(true, x + 12, y + 5);
		SetCell(true, x + 13, y + 3);
		SetCell(true, x + 13, y + 4);
		SetCell(true, x + 13, y + 8);
		SetCell(true, x + 13, y + 9);

		SetCell(true, x + 27, y + 8);
		SetCell(true, x + 28, y + 7);
		SetCell(true, x + 28, y + 8);
		SetCell(true, x + 28, y + 9);
		SetCell(true, x + 29, y + 6);
		SetCell(true, x + 29, y + 7);
		SetCell(true, x + 29, y + 8);
		SetCell(true, x + 29, y + 9);
		SetCell(true, x + 29, y + 10);
		SetCell(true, x + 30, y + 5);
		SetCell(true, x + 30, y + 6);
		SetCell(true, x + 30, y + 10);
		SetCell(true, x + 30, y + 11);

		SetCell(true, x + 34, y + 7);
		SetCell(true, x + 34, y + 8);
		SetCell(true, x + 34, y + 9);
		SetCell(true, x + 35, y + 7);
		SetCell(true, x + 35, y + 8);
		SetCell(true, x + 35, y + 9);

		SetCell(true, x + 38, y + 8);
		SetCell(true, x + 38, y + 9);
		SetCell(true, x + 39, y + 8);
		SetCell(true, x + 39, y + 9);
	}

	GameOfLife()
	{
		InitConsole();
	}

	void InitGame()
	{
		auto t = std::thread(&GameOfLife::GameThread, this);
		t.join();
	}
};

int main()
{
	GameOfLife gof;
	gof.InitGame();
	
	return 0;
}
