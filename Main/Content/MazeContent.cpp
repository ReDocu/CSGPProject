#include "MazeContent.h"
#include "../../framework.h"

// ============================================================================
// Direction unit vectors: 0 = North, 1 = East, 2 = South, 3 = West.
// Used both as cell deltas (generation) and grid deltas (movement/pathfinding);
// numerically identical unit vectors.
// ============================================================================
static const int DX[4] = { 0, 1, 0, -1 };
static const int DY[4] = { -1, 0, 1, 0 };

static int uf_find(int* parent, int a)
{
	while (parent[a] != a)
	{
		parent[a] = parent[parent[a]];	// path compression (halving)
		a = parent[a];
	}
	return a;
}

// ============================================================================
// Lifecycle
// ============================================================================
void MazeContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;
	totalScore = 0;
	stage = 0;
	genAlgo = _EGenAlgo::BINARY_TREE;
	solveAlgo = _ESolveAlgo::BFS;
	gameState = _EGameState::PLAYING;

	steps = 0;
	optimalSteps = 0;
	stageScore = 0;
	hintsUsed = 0;
	stars = 0;
	usedSolve = false;
	seed = 0;
	hintBlink = 0;
	solveAnimIdx = 0;
	moveNextTime = 0.0f;

	hintCells.clear();
	solveOrder.clear();
	solvePath.clear();
}

void MazeContent::OnRelease()
{
	IGameContent::OnRelease();
	hintCells.clear();
	solveOrder.clear();
	solvePath.clear();
}

// ============================================================================
// Stage setup
// ============================================================================
void MazeContent::StartStage(int stageIndex)
{
	stage = stageIndex;
	genAlgo = (_EGenAlgo)stageIndex;

	// capture a seed so the maze is reproducible and can be shown / re-rolled
	seed = (unsigned int)rand() ^ ((unsigned int)rand() << 15);
	srand(seed);

	sgx = 1; sgy = 1;					// entrance cell (0,0) -> grid (1,1)
	egx = GRID_W - 2; egy = GRID_H - 2;	// exit cell (COLS-1,ROWS-1)

	// generate, verify reachable, re-roll a few times if needed (safety net)
	std::vector<int> tmp;
	int startCell = sgy * GRID_W + sgx;
	int goalCell = egy * GRID_W + egx;
	for (int attempt = 0; attempt < 6; attempt++)
	{
		GenerateMaze(genAlgo);
		optimalSteps = ShortestPath(startCell, goalCell, tmp);
		if (optimalSteps > 0)
			break;
	}

	pgx = sgx; pgy = sgy;
	steps = 0;
	hintsUsed = 0;
	usedSolve = false;
	hintBlink = 0;
	hintCells.clear();
	solveOrder.clear();
	solvePath.clear();
	solveAnimIdx = 0;

	gameState = _EGameState::PLAYING;
	moveNextTime = 0.0f;
	TIMER->StartContent();
}

// ============================================================================
// Generation dispatch (cell space -> writes wall grid)
// ============================================================================
void MazeContent::GenerateMaze(_EGenAlgo algo)
{
	switch (algo)
	{
	case _EGenAlgo::BINARY_TREE: GenBinaryTree(); break;
	case _EGenAlgo::BACKTRACKER: GenBacktracker(); break;
	case _EGenAlgo::PRIM:        GenPrim();        break;
	case _EGenAlgo::KRUSKAL:     GenKruskal();     break;
	case _EGenAlgo::DIVISION:    // handled below (needs boundary + recursion)
	default:
	{
		for (int gy = 0; gy < GRID_H; gy++)
			for (int gx = 0; gx < GRID_W; gx++)
				wall[gy][gx] = (gx == 0 || gy == 0 || gx == GRID_W - 1 || gy == GRID_H - 1);
		GenDivision(0, 0, GRID_W - 1, GRID_H - 1);
		break;
	}
	}
}

void MazeContent::ResetWallsAll(bool val)
{
	for (int gy = 0; gy < GRID_H; gy++)
		for (int gx = 0; gx < GRID_W; gx++)
			wall[gy][gx] = val;
}

void MazeContent::OpenAllCells()
{
	for (int cy = 0; cy < CELL_ROWS; cy++)
		for (int cx = 0; cx < CELL_COLS; cx++)
			wall[2 * cy + 1][2 * cx + 1] = false;
}

// Stage 1 - Binary Tree: for each cell carve North or East. Strong diagonal bias.
void MazeContent::GenBinaryTree()
{
	ResetWallsAll(true);
	OpenAllCells();

	for (int cy = 0; cy < CELL_ROWS; cy++)
	{
		for (int cx = 0; cx < CELL_COLS; cx++)
		{
			bool canN = (cy > 0);
			bool canE = (cx < CELL_COLS - 1);
			if (!canN && !canE)
				continue;

			bool carveEast = canE;
			if (canN && canE)
				carveEast = (rand() % 2 == 0);

			if (carveEast)
				wall[2 * cy + 1][2 * cx + 2] = false;	// open east wall
			else
				wall[2 * cy][2 * cx + 1] = false;		// open north wall
		}
	}
}

// Stage 2 - Recursive Backtracker (DFS): explicit stack. Long winding corridors.
void MazeContent::GenBacktracker()
{
	ResetWallsAll(true);
	OpenAllCells();

	bool visited[NUM_CELLS];
	for (int i = 0; i < NUM_CELLS; i++)
		visited[i] = false;

	std::vector<int> st;
	visited[0] = true;
	st.push_back(0);

	while (!st.empty())
	{
		int cur = st.back();
		int cx = cur % CELL_COLS;
		int cy = cur / CELL_COLS;

		int cand[4];
		int nc = 0;
		for (int d = 0; d < 4; d++)
		{
			int nx = cx + DX[d];
			int ny = cy + DY[d];
			if (nx < 0 || nx >= CELL_COLS || ny < 0 || ny >= CELL_ROWS)
				continue;
			if (!visited[ny * CELL_COLS + nx])
				cand[nc++] = d;
		}

		if (nc == 0)
		{
			st.pop_back();
			continue;
		}

		int d = cand[rand() % nc];
		int nx = cx + DX[d];
		int ny = cy + DY[d];
		wall[2 * cy + 1 + DY[d]][2 * cx + 1 + DX[d]] = false;	// carve between
		visited[ny * CELL_COLS + nx] = true;
		st.push_back(ny * CELL_COLS + nx);
	}
}

// Stage 3 - Randomized Prim: frontier set. Short, bushy passages.
void MazeContent::GenPrim()
{
	ResetWallsAll(true);
	OpenAllCells();

	bool inMaze[NUM_CELLS];
	bool inFront[NUM_CELLS];
	for (int i = 0; i < NUM_CELLS; i++)
	{
		inMaze[i] = false;
		inFront[i] = false;
	}

	std::vector<int> front;
	inMaze[0] = true;

	// seed frontier with start's neighbors
	for (int d = 0; d < 4; d++)
	{
		int nx = DX[d];
		int ny = DY[d];
		if (nx < 0 || nx >= CELL_COLS || ny < 0 || ny >= CELL_ROWS)
			continue;
		int ni = ny * CELL_COLS + nx;
		inFront[ni] = true;
		front.push_back(ni);
	}

	while (!front.empty())
	{
		int fi = rand() % (int)front.size();
		int f = front[fi];
		front[fi] = front.back();
		front.pop_back();
		inFront[f] = false;

		int cx = f % CELL_COLS;
		int cy = f / CELL_COLS;

		// connect to a random neighbor already in the maze
		int mcand[4];
		int mc = 0;
		for (int d = 0; d < 4; d++)
		{
			int nx = cx + DX[d];
			int ny = cy + DY[d];
			if (nx < 0 || nx >= CELL_COLS || ny < 0 || ny >= CELL_ROWS)
				continue;
			if (inMaze[ny * CELL_COLS + nx])
				mcand[mc++] = d;
		}
		if (mc == 0)
			continue;

		int d = mcand[rand() % mc];
		wall[2 * cy + 1 + DY[d]][2 * cx + 1 + DX[d]] = false;
		inMaze[f] = true;

		// push f's outside neighbors into the frontier
		for (int d2 = 0; d2 < 4; d2++)
		{
			int nx = cx + DX[d2];
			int ny = cy + DY[d2];
			if (nx < 0 || nx >= CELL_COLS || ny < 0 || ny >= CELL_ROWS)
				continue;
			int ni = ny * CELL_COLS + nx;
			if (!inMaze[ni] && !inFront[ni])
			{
				inFront[ni] = true;
				front.push_back(ni);
			}
		}
	}
}

// Stage 4 - Kruskal + Union-Find: shuffle all walls, remove if it joins two sets.
void MazeContent::GenKruskal()
{
	ResetWallsAll(true);
	OpenAllCells();

	int parent[NUM_CELLS];
	for (int i = 0; i < NUM_CELLS; i++)
		parent[i] = i;

	// edge = wall between two adjacent cells
	struct Edge { int a, b, wx, wy; };
	std::vector<Edge> edges;
	for (int cy = 0; cy < CELL_ROWS; cy++)
	{
		for (int cx = 0; cx < CELL_COLS; cx++)
		{
			int idx = cy * CELL_COLS + cx;
			if (cx < CELL_COLS - 1)
			{
				Edge e; e.a = idx; e.b = idx + 1;
				e.wx = 2 * cx + 2; e.wy = 2 * cy + 1;
				edges.push_back(e);
			}
			if (cy < CELL_ROWS - 1)
			{
				Edge e; e.a = idx; e.b = idx + CELL_COLS;
				e.wx = 2 * cx + 1; e.wy = 2 * cy + 2;
				edges.push_back(e);
			}
		}
	}

	// Fisher-Yates shuffle
	for (int i = (int)edges.size() - 1; i > 0; i--)
	{
		int j = rand() % (i + 1);
		Edge t = edges[i]; edges[i] = edges[j]; edges[j] = t;
	}

	for (size_t i = 0; i < edges.size(); i++)
	{
		int ra = uf_find(parent, edges[i].a);
		int rb = uf_find(parent, edges[i].b);
		if (ra != rb)
		{
			wall[edges[i].wy][edges[i].wx] = false;
			parent[ra] = rb;
		}
	}
}

// Stage 5 - Recursive Division: start open, add walls with a single gap each.
void MazeContent::GenDivision(int gx0, int gy0, int gx1, int gy1)
{
	int w = gx1 - gx0;
	int h = gy1 - gy0;
	if (w < 4 && h < 4)
		return;

	bool horizontal;
	if (w < 4)       horizontal = true;
	else if (h < 4)  horizontal = false;
	else if (h > w)  horizontal = true;
	else if (w > h)  horizontal = false;
	else             horizontal = (rand() % 2 == 0);

	if (horizontal)
	{
		int slots = (h - 2) / 2;					// candidate even rows
		int wy = gy0 + 2 + 2 * (rand() % slots);	// wall row (even)
		int hx = gx0 + 1 + 2 * (rand() % (w / 2));	// gap column (odd cell)
		for (int x = gx0 + 1; x <= gx1 - 1; x++)
			wall[wy][x] = true;
		wall[wy][hx] = false;

		GenDivision(gx0, gy0, gx1, wy);
		GenDivision(gx0, wy, gx1, gy1);
	}
	else
	{
		int slots = (w - 2) / 2;
		int wx = gx0 + 2 + 2 * (rand() % slots);	// wall column (even)
		int hy = gy0 + 1 + 2 * (rand() % (h / 2));	// gap row (odd cell)
		for (int y = gy0 + 1; y <= gy1 - 1; y++)
			wall[y][wx] = true;
		wall[hy][wx] = false;

		GenDivision(gx0, gy0, wx, gy1);
		GenDivision(wx, gy0, gx1, gy1);
	}
}

// ============================================================================
// Pathfinding / movement (grid space)
// ============================================================================
bool MazeContent::GridOpen(int gx, int gy, int dir) const
{
	int nx = gx + DX[dir];
	int ny = gy + DY[dir];
	if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H)
		return false;
	return !wall[ny][nx];	// passable if the adjacent grid cell is not a wall
}

int MazeContent::ShortestPath(int startCell, int goalCell, std::vector<int>& pathOut) const
{
	pathOut.clear();

	int prev[NUM_GRID];
	bool vis[NUM_GRID];
	for (int i = 0; i < NUM_GRID; i++) { prev[i] = -1; vis[i] = false; }

	std::vector<int> q;
	size_t head = 0;
	vis[startCell] = true;
	q.push_back(startCell);

	while (head < q.size())
	{
		int cur = q[head++];
		if (cur == goalCell)
			break;
		int gx = cur % GRID_W;
		int gy = cur / GRID_W;
		for (int d = 0; d < 4; d++)
		{
			if (!GridOpen(gx, gy, d))
				continue;
			int ni = (gy + DY[d]) * GRID_W + (gx + DX[d]);
			if (!vis[ni])
			{
				vis[ni] = true;
				prev[ni] = cur;
				q.push_back(ni);
			}
		}
	}

	if (!vis[goalCell])
		return -1;

	// reconstruct (reversed)
	int c = goalCell;
	while (c != -1) { pathOut.push_back(c); c = prev[c]; }
	for (size_t i = 0, j = pathOut.size() - 1; i < j; i++, j--)
	{
		int t = pathOut[i]; pathOut[i] = pathOut[j]; pathOut[j] = t;
	}
	return (int)pathOut.size() - 1;
}

void MazeContent::BuildSolve(_ESolveAlgo algo)
{
	solveOrder.clear();
	solvePath.clear();
	solveAnimIdx = 0;

	int start = sgy * GRID_W + sgx;
	int goal = egy * GRID_W + egx;

	if (algo == _ESolveAlgo::WALL_FOLLOWER)
	{
		int cur = start;
		int facing = 1;	// start facing east
		solveOrder.push_back(cur);
		solvePath.push_back(cur);
		int cap = NUM_GRID * 8;
		const int turn[4] = { 1, 0, 3, 2 };	// right, straight, left, back
		while (cur != goal && cap-- > 0)
		{
			int gx = cur % GRID_W;
			int gy = cur / GRID_W;
			for (int t = 0; t < 4; t++)
			{
				int nd = (facing + turn[t]) % 4;
				if (GridOpen(gx, gy, nd))
				{
					facing = nd;
					cur = (gy + DY[nd]) * GRID_W + (gx + DX[nd]);
					solveOrder.push_back(cur);
					solvePath.push_back(cur);
					break;
				}
			}
		}
		return;
	}

	// BFS / DFS / A* share reconstruction via prev
	int prev[NUM_GRID];
	bool vis[NUM_GRID];
	for (int i = 0; i < NUM_GRID; i++) { prev[i] = -1; vis[i] = false; }

	if (algo == _ESolveAlgo::BFS)
	{
		std::vector<int> q;
		size_t head = 0;
		vis[start] = true;
		q.push_back(start);
		while (head < q.size())
		{
			int cur = q[head++];
			solveOrder.push_back(cur);
			if (cur == goal) break;
			int gx = cur % GRID_W, gy = cur / GRID_W;
			for (int d = 0; d < 4; d++)
			{
				if (!GridOpen(gx, gy, d)) continue;
				int ni = (gy + DY[d]) * GRID_W + (gx + DX[d]);
				if (!vis[ni]) { vis[ni] = true; prev[ni] = cur; q.push_back(ni); }
			}
		}
	}
	else if (algo == _ESolveAlgo::DFS)
	{
		std::vector<int> stk;
		stk.push_back(start);
		while (!stk.empty())
		{
			int cur = stk.back();
			stk.pop_back();
			if (vis[cur]) continue;
			vis[cur] = true;
			solveOrder.push_back(cur);
			if (cur == goal) break;
			int gx = cur % GRID_W, gy = cur / GRID_W;
			for (int d = 0; d < 4; d++)
			{
				if (!GridOpen(gx, gy, d)) continue;
				int ni = (gy + DY[d]) * GRID_W + (gx + DX[d]);
				if (!vis[ni])
				{
					if (prev[ni] == -1 && ni != start) prev[ni] = cur;
					stk.push_back(ni);
				}
			}
		}
	}
	else // A*
	{
		int g[NUM_GRID];
		bool closed[NUM_GRID];
		for (int i = 0; i < NUM_GRID; i++) { g[i] = 1 << 29; closed[i] = false; }
		int ggx = goal % GRID_W, ggy = goal / GRID_W;

		std::vector<int> open;
		g[start] = 0;
		open.push_back(start);
		while (!open.empty())
		{
			int bi = 0, bf = 1 << 30;
			for (size_t i = 0; i < open.size(); i++)
			{
				int c = open[i];
				int gx = c % GRID_W, gy = c / GRID_W;
				int h = (gx > ggx ? gx - ggx : ggx - gx) + (gy > ggy ? gy - ggy : ggy - gy);
				int f = g[c] + h;
				if (f < bf) { bf = f; bi = (int)i; }
			}
			int cur = open[bi];
			open[bi] = open.back();
			open.pop_back();
			if (closed[cur]) continue;
			closed[cur] = true;
			solveOrder.push_back(cur);
			if (cur == goal) break;
			int gx = cur % GRID_W, gy = cur / GRID_W;
			for (int d = 0; d < 4; d++)
			{
				if (!GridOpen(gx, gy, d)) continue;
				int ni = (gy + DY[d]) * GRID_W + (gx + DX[d]);
				int tentative = g[cur] + 1;
				if (tentative < g[ni])
				{
					g[ni] = tentative;
					prev[ni] = cur;
					open.push_back(ni);
				}
			}
		}
	}

	// reconstruct route
	int c = goal;
	if (prev[goal] != -1 || goal == start)
	{
		std::vector<int> rev;
		while (c != -1) { rev.push_back(c); c = prev[c]; }
		for (int i = (int)rev.size() - 1; i >= 0; i--)
			solvePath.push_back(rev[i]);
	}
}

void MazeContent::BuildHint()
{
	std::vector<int> path;
	int startCell = pgy * GRID_W + pgx;
	int goalCell = egy * GRID_W + egx;
	ShortestPath(startCell, goalCell, path);

	hintCells.clear();
	for (size_t i = 1; i < path.size() && i <= 8; i++)
		hintCells.push_back(path[i]);
}

// ============================================================================
// Play
// ============================================================================
void MazeContent::MovePlayer(int dir)
{
	if (!GridOpen(pgx, pgy, dir))
		return;
	pgx += DX[dir];
	pgy += DY[dir];
	steps++;
	if (pgx == egx && pgy == egy)
		FinishStage();
}

void MazeContent::FinishStage()
{
	float ratio = (steps > 0) ? (float)optimalSteps / (float)steps : 1.0f;
	if (ratio > 1.0f) ratio = 1.0f;

	if (usedSolve)          stars = 1;
	else if (ratio >= 0.9f) stars = 3;
	else if (ratio >= 0.7f) stars = 2;
	else                    stars = 1;

	int effBonus = usedSolve ? 0 : (int)(1000.0f * ratio);
	int t = (int)TIMER->GetContentTime();
	int timeBonus = 300 - t * 3;
	if (timeBonus < 0) timeBonus = 0;

	stageScore = 1000 + effBonus + timeBonus - hintsUsed * 100;
	if (stageScore < 0) stageScore = 0;

	totalScore += stageScore;
	if (totalScore > hiScore) hiScore = totalScore;

	gameState = _EGameState::CLEAR;
}

// ============================================================================
// Title
// ============================================================================
void MazeContent::OnTitleUpdate()
{
	if (INPUT->OnKeyDown(VK_RETURN))
	{
		totalScore = 0;
		StartStage(0);
		currentPhase = _EPhase::INGAME;
	}
}

void MazeContent::OnTitleRender()
{
	SCREEN->OnDrawColor(28, 3, "A L G O   M A Z E", SKYBLUE);
	SCREEN->OnDraw(22, 5, "Each stage is built by a different algorithm");

	SCREEN->OnDrawColor(24, 8,  "STAGE 1  Binary Tree      (bias)",       DARKGRAY);
	SCREEN->OnDrawColor(24, 9,  "STAGE 2  Recursive DFS     (stack)",     DARKBLUE);
	SCREEN->OnDrawColor(24, 10, "STAGE 3  Randomized Prim   (frontier)",  DARKGREEN);
	SCREEN->OnDrawColor(24, 11, "STAGE 4  Kruskal           (union-find)",DARKPURPLE);
	SCREEN->OnDrawColor(24, 12, "STAGE 5  Recursive Division(divide)",    DARKRED);

	SCREEN->OnDraw(26, 15, "Reach the exit. Fewer steps = more stars.");
	SCREEN->OnDraw(30, 17, "PRESS ENTER TO START");
	SCREEN->OnDraw(34, 19, "ESC: QUIT");
}

// ============================================================================
// In-game update
// ============================================================================
void MazeContent::OnInGameUpdate()
{
	// ----- CLEAR -----
	if (gameState == _EGameState::CLEAR)
	{
		if (INPUT->OnKeyDown(VK_RETURN))
		{
			if (stage < (int)_EGenAlgo::GEN_COUNT - 1)
				StartStage(stage + 1);
			else
				currentPhase = _EPhase::TITLE;
			return;
		}
		if (INPUT->OnKeyDown(VK_ESCAPE))
			currentPhase = _EPhase::TITLE;
		return;
	}

	// ----- SOLVING (visualization) -----
	if (gameState == _EGameState::SOLVING)
	{
		if (INPUT->OnKeyDown(VK_TAB))
		{
			solveAlgo = (_ESolveAlgo)(((int)solveAlgo + 1) % (int)_ESolveAlgo::SOLVE_COUNT);
			BuildSolve(solveAlgo);
			return;
		}
		if (INPUT->OnKeyDown(VK_RETURN) || INPUT->OnKeyDown(VK_SPACE) ||
			INPUT->OnKeyDown('F') || INPUT->OnKeyDown(VK_ESCAPE))
		{
			gameState = _EGameState::PLAYING;
			return;
		}
		if (TIMER->GetTickTimer(0.02f))
			solveAnimIdx++;
		return;
	}

	// ----- ESC -> title -----
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		currentPhase = _EPhase::TITLE;
		return;
	}

	// ----- pause toggle -----
	if (INPUT->OnKeyDown('P'))
	{
		if (gameState == _EGameState::PLAYING)
			gameState = _EGameState::PAUSE;
		else if (gameState == _EGameState::PAUSE)
			gameState = _EGameState::PLAYING;
	}
	if (gameState == _EGameState::PAUSE)
		return;

	// ----- PLAYING -----
	if (hintBlink > 0)
		hintBlink--;

	if (INPUT->OnKeyDown(VK_TAB))
		solveAlgo = (_ESolveAlgo)(((int)solveAlgo + 1) % (int)_ESolveAlgo::SOLVE_COUNT);

	if (INPUT->OnKeyDown('R'))
	{
		StartStage(stage);
		return;
	}

	if (INPUT->OnKeyDown('H'))
	{
		BuildHint();
		hintsUsed++;
		hintBlink = 60;
	}

	if (INPUT->OnKeyDown('F'))
	{
		BuildSolve(solveAlgo);
		usedSolve = true;
		solveAnimIdx = 0;
		gameState = _EGameState::SOLVING;
		return;
	}

	// movement: one step per press; holding repeats after a short initial delay
	float moveNow = TIMER->GetContentTime();
	int moveDir = -1;
	if (INPUT->OnKeyDown(VK_UP))         { moveDir = 0; moveNextTime = moveNow + 0.22f; }
	else if (INPUT->OnKeyDown(VK_RIGHT)) { moveDir = 1; moveNextTime = moveNow + 0.22f; }
	else if (INPUT->OnKeyDown(VK_DOWN))  { moveDir = 2; moveNextTime = moveNow + 0.22f; }
	else if (INPUT->OnKeyDown(VK_LEFT))  { moveDir = 3; moveNextTime = moveNow + 0.22f; }
	else if (moveNow >= moveNextTime)
	{
		if (INPUT->OnKeyStay(VK_UP))         moveDir = 0;
		else if (INPUT->OnKeyStay(VK_RIGHT)) moveDir = 1;
		else if (INPUT->OnKeyStay(VK_DOWN))  moveDir = 2;
		else if (INPUT->OnKeyStay(VK_LEFT))  moveDir = 3;
		if (moveDir >= 0) moveNextTime = moveNow + 0.10f;
	}
	if (moveDir >= 0) MovePlayer(moveDir);
}

// ============================================================================
// In-game render
// ============================================================================
void MazeContent::OnInGameRender()
{
	DrawMaze();
	DrawStartExit();

	if (gameState == _EGameState::SOLVING)
		DrawSolveOverlay();
	else
		DrawHintOverlay();

	DrawPlayer();
	DrawHUD();

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(20, 10, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::CLEAR)
		DrawClearOverlay();
}

void MazeContent::DrawGridGlyph(int gx, int gy, const char* glyph, int color)
{
	SCREEN->OnDrawColor((MAZE_X0 + gx) * 2, MAZE_Y0 + gy, glyph, color);
}

void MazeContent::DrawMaze()
{
	int col = WallColor();
	// draw a whole grid row as one string (fewer console calls than per-cell)
	for (int gy = 0; gy < GRID_H; gy++)
	{
		std::string row;
		for (int gx = 0; gx < GRID_W; gx++)
			row += wall[gy][gx] ? "¡á" : "  ";
		SCREEN->OnDrawColor((MAZE_X0 + 0) * 2, MAZE_Y0 + gy, row.c_str(), col);
	}
}

void MazeContent::DrawStartExit()
{
	DrawGridGlyph(sgx, sgy, "¡ß", GREEN);
	DrawGridGlyph(egx, egy, "¡Ú", RED);
}

void MazeContent::DrawPlayer()
{
	DrawGridGlyph(pgx, pgy, "¡Ü", YELLOW);
}

void MazeContent::DrawHintOverlay()
{
	if (hintBlink <= 0)
		return;
	if ((hintBlink / 5) % 2 != 0)
		return;
	for (size_t i = 0; i < hintCells.size(); i++)
	{
		int cell = hintCells[i];
		DrawGridGlyph(cell % GRID_W, cell / GRID_W, "¡Ü", PURPLE);
	}
}

void MazeContent::DrawSolveOverlay()
{
	int start = sgy * GRID_W + sgx;
	int goal = egy * GRID_W + egx;

	int orderShown = solveAnimIdx;
	if (orderShown > (int)solveOrder.size())
		orderShown = (int)solveOrder.size();

	for (int i = 0; i < orderShown; i++)
	{
		int cell = solveOrder[i];
		if (cell == start || cell == goal)
			continue;
		DrawGridGlyph(cell % GRID_W, cell / GRID_W, "¡Ü", DARKSKYBLUE);
	}

	if (solveAnimIdx > (int)solveOrder.size())
	{
		int pathShown = solveAnimIdx - (int)solveOrder.size();
		if (pathShown > (int)solvePath.size())
			pathShown = (int)solvePath.size();
		for (int i = 0; i < pathShown; i++)
		{
			int cell = solvePath[i];
			if (cell == start || cell == goal)
				continue;
			DrawGridGlyph(cell % GRID_W, cell / GRID_W, "¡Ü", YELLOW);
		}
	}

	SCREEN->OnDrawColor(HUD_X, 23, "SOLVING...", SKYBLUE);
}

void MazeContent::DrawClearOverlay()
{
	SCREEN->OnDrawColor(20, 8, "S T A G E   C L E A R", GREEN);

	std::string starStr;
	for (int i = 0; i < stars; i++)
		starStr += "¡Ú";
	SCREEN->OnDrawColor(28, 10, starStr.c_str(), YELLOW);

	std::string line1 = "STEPS " + std::to_string(steps) + "   BEST " + std::to_string(optimalSteps);
	SCREEN->OnDraw(24, 12, line1.c_str());
	std::string line2 = "STAGE SCORE " + std::to_string(stageScore);
	SCREEN->OnDraw(24, 13, line2.c_str());

	if (stage < (int)_EGenAlgo::GEN_COUNT - 1)
		SCREEN->OnDraw(24, 15, "ENTER: NEXT STAGE");
	else
		SCREEN->OnDraw(22, 15, "ALL CLEAR!   ENTER: TITLE");
}

void MazeContent::DrawHUD()
{
	SCREEN->OnDrawColor(HUD_X, 1, "ALGO MAZE", SKYBLUE);

	std::string st = "STAGE " + std::to_string(stage + 1) + "/5";
	SCREEN->OnDraw(HUD_X, 3, st.c_str());

	SCREEN->OnDrawColor(HUD_X, 4, "GEN:", WHITE);
	SCREEN->OnDrawColor(HUD_X, 5, GenAlgoName(genAlgo), YELLOW);

	SCREEN->OnDrawColor(HUD_X, 7, "SOLVER:", WHITE);
	SCREEN->OnDrawColor(HUD_X, 8, SolveAlgoName(solveAlgo), SKYBLUE);

	std::string ln;
	ln = "STEPS " + std::to_string(steps);          SCREEN->OnDraw(HUD_X, 10, ln.c_str());
	ln = "BEST  " + std::to_string(optimalSteps);   SCREEN->OnDraw(HUD_X, 11, ln.c_str());
	ln = "SCORE " + std::to_string(totalScore);     SCREEN->OnDraw(HUD_X, 12, ln.c_str());
	ln = "SEED  " + std::to_string(seed % 100000);  SCREEN->OnDraw(HUD_X, 13, ln.c_str());

	SCREEN->OnDraw(HUD_X, 16, "ARROWS: MOVE");
	SCREEN->OnDraw(HUD_X, 17, "H:HINT  F:SOLVE");
	SCREEN->OnDraw(HUD_X, 18, "TAB: SOLVER");
	SCREEN->OnDraw(HUD_X, 19, "R: NEW MAZE");
	SCREEN->OnDraw(HUD_X, 20, "P:PAUSE ESC:TITLE");
}

int MazeContent::WallColor() const
{
	switch (genAlgo)
	{
	case _EGenAlgo::BINARY_TREE: return DARKGRAY;
	case _EGenAlgo::BACKTRACKER: return DARKBLUE;
	case _EGenAlgo::PRIM:        return DARKGREEN;
	case _EGenAlgo::KRUSKAL:     return DARKPURPLE;
	case _EGenAlgo::DIVISION:    return DARKRED;
	default:                     return DARKGRAY;
	}
}

const char* MazeContent::GenAlgoName(_EGenAlgo a) const
{
	switch (a)
	{
	case _EGenAlgo::BINARY_TREE: return "Binary Tree";
	case _EGenAlgo::BACKTRACKER: return "Recursive DFS";
	case _EGenAlgo::PRIM:        return "Randomized Prim";
	case _EGenAlgo::KRUSKAL:     return "Kruskal (UF)";
	case _EGenAlgo::DIVISION:    return "Rec. Division";
	default:                     return "?";
	}
}

const char* MazeContent::SolveAlgoName(_ESolveAlgo a) const
{
	switch (a)
	{
	case _ESolveAlgo::BFS:           return "BFS";
	case _ESolveAlgo::DFS:           return "DFS";
	case _ESolveAlgo::ASTAR:         return "A-Star";
	case _ESolveAlgo::WALL_FOLLOWER: return "Wall Follower";
	default:                         return "?";
	}
}
