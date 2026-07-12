#pragma once

// ============================================================================
// Generic object pool (curriculum: Object Pool).
//
// Fixed capacity N, reused via the 'active' flag. Short-lived objects
// (bullets / enemies / explosions / items) live here so the game loop never
// calls new / delete / vector-rebuild - allocation cost is paid once.
//
//   Spawn() : borrow a free slot (linear scan; -1 if full)
//   Kill(i) : release a slot (mark inactive, no deallocation)
//   Clear() : release every slot (restart / stage change)
//
// Shared by GalagaContent and BattleCityContent. ASCII header - edit-safe.
// ============================================================================
template <typename T, int N>
struct Pool
{
	T    items[N];
	bool active[N];

	Pool()       { for (int i = 0; i < N; i++) active[i] = false; }
	void Clear() { for (int i = 0; i < N; i++) active[i] = false; }

	int Spawn()
	{
		for (int i = 0; i < N; i++)
			if (!active[i]) { active[i] = true; return i; }
		return -1;
	}
	void Kill(int i) { active[i] = false; }
};
