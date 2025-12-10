#pragma once
#include <windows.h>
#include <vector>

struct VillageHouse {
    int x, y, w, h, type;// 0: normal, 1: geniþ
    int id; 
};
struct VillageTree {
    int x, y, size;
    int type; // 0: normal, 1: kavak
};


class VillageBackground {
public:
    VillageBackground(int width, int height);
    ~VillageBackground();
    void Draw(HDC hdc, int cameraX, int cameraY, int screenW, int screenH, float zoom = 1.0f);
    void Update();
    const std::vector<RECT>& GetObstacles() const;
    const std::vector<VillageHouse>& GetHouses() const;

   
    void InitializeMazeArray();
    void UpdateMazeArray();
    bool IsWalkable(int x, int y) const;
    int GetMazeValue(int x, int y) const;

private:
    int m_width, m_height;
    std::vector<VillageTree> m_trees;
    std::vector<VillageHouse> m_houses;
    std::vector<RECT> m_obstacles;

   
    int** m_mazeArray;  // 2D array for pathfinding (0 = walkable, 1 = obstacle)
};

