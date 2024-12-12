#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <cmath>
#include <map>
#include "raylib.h"

enum PieceType {
    none,
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
};
enum PieceColor {
    unknownColor,
    black,
    white
};
enum GameState {
    unknownState,
    whiteTurn,
    blackTurn,
};

class Piece {
public:
    Piece() : color{ PieceColor::unknownColor }, pieceType{ PieceType::none }
    {
    }
    Piece(PieceType type_, PieceColor color_) : color(color_), pieceType(type_)
    {
    }

    void setPieceType(PieceType type_) {
        pieceType = type_;
    }
    void setColor(PieceColor color_) {
        color = color_;
    }

    PieceType getType() const { return pieceType; }
    PieceColor getColor() const { return color; }

private:
    PieceColor color{};
    PieceType pieceType{};
};

class Tile {
public:
    Tile(int row_, int column_) : row{row_}, column{column_}, piece{std::nullopt}
    { }

    void setPiece(PieceType type_, PieceColor color_) {
        piece = Piece(type_, color_);
    }

    void removePiece() {
        piece = std::nullopt;
    }

    bool hasPiece() const {
        return piece.has_value();
    }
    const std::optional<Piece>& getPiece() const { return piece; }

private:
    int row {};
    int column {};
    std::optional<Piece> piece {};
};


class Board {
public:
    Board(const int size = 8) : size(size), gameState{GameState::whiteTurn} {
        for (int y = 0; y < size; ++y) {
            std::vector<Tile> row;
            for (int x = 0; x < size; ++x) {
                row.emplace_back(Tile(x, y));
            }
            board.push_back(row);
        }
    }

    bool isTurnValid(PieceColor pieceColor) const {
        return (gameState == GameState::whiteTurn && pieceColor == PieceColor::white) ||
               (gameState == GameState::blackTurn && pieceColor == PieceColor::black);
    }

    void switchTurn() {
        gameState = (gameState == GameState::whiteTurn) ? GameState::blackTurn : GameState::whiteTurn;
    }

    Tile& getTile(int x, int y) {
        if (x < 0 || x >= board.size() || y < 0 || y >= board[0].size()) {
            throw std::out_of_range("Invalid tile coordinates");
        }
        return board[y][x];
    }

    void placePiece(int x, int y, PieceType type, PieceColor color) {
        getTile(x, y).setPiece(type, color);
    }
    void removePiece(int x, int y) {
        getTile(x, y).removePiece();
    }
    bool isPathClear(int startX, int startY, int endX, int endY) {
        int deltaX = (endX - startX) == 0 ? 0 : (endX - startX) / abs(endX - startX);
        int deltaY = (endY - startY) == 0 ? 0 : (endY - startY) / abs(endY - startY);

        int x = startX + deltaX;
        int y = startY + deltaY;

        while (x != endX || y != endY) {
            if (getTile(x, y).hasPiece()) {
                return false;
            }
            x += deltaX;
            y += deltaY;
        }
        return true;
    }

    bool isMoveValid(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        int deltaX = abs(endX - startX);
        int deltaY = abs(endY - startY);

        switch (pieceType) {
        case pawn:
            if (pieceColor == PieceColor::white) {
                
                if (startY == 1 && endY - startY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) return true; 
                if (endY - startY == 1 && deltaX == 0 && !getTile(endX, endY).hasPiece()) return true; 

                
                if (endY - startY == 1 && deltaX == 1 && getTile(endX, endY).hasPiece() &&
                    getTile(endX, endY).getPiece()->getColor() == PieceColor::black) return true;
            }
            else if (pieceColor == PieceColor::black) {
                
                if (startY == 6 && startY - endY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) return true; 
                if (startY - endY == 1 && deltaX == 0 && !getTile(endX, endY).hasPiece()) return true; 

                
                if (startY - endY == 1 && deltaX == 1 && getTile(endX, endY).hasPiece() &&
                    getTile(endX, endY).getPiece()->getColor() == PieceColor::white) return true;
            }
            return false;

        case knight:
            return (deltaX == 2 && deltaY == 1) || (deltaX == 1 && deltaY == 2);

        case bishop:
            return deltaX == deltaY && isPathClear(startX, startY, endX, endY);

        case rook:
            return ((deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0)) && isPathClear(startX, startY, endX, endY);

        case queen:
            return ((deltaX == deltaY) || (deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0)) && isPathClear(startX, startY, endX, endY);

        case king:
            return deltaX <= 1 && deltaY <= 1;

        default:
            return false;
        }
    }




    bool checkForObstaclesAtDestanationTile(int endX, int endY, PieceColor pieceColor) {
        if (!getTile(endX, endY).hasPiece()) {
            return true;
        }
        else {
            Piece piece = getTile(endX, endY).getPiece().value();
            if (piece.getColor() == pieceColor) {
                return false;
            }
            else {
                return true;
            }
        }
        return false;
    }

    bool validate(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        if (isMoveValid(startX, startY, endX, endY, pieceColor, pieceType) && isTurnValid(pieceColor) && checkForObstaclesAtDestanationTile(endX, endY, pieceColor)) {
            return true;
        }
        return false;
    }

    void makeMove(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        if (validate(startX, startY, endX, endY, pieceColor, pieceType)) {
            getTile(startX, startY).removePiece();
            getTile(endX, endY).setPiece(pieceType, pieceColor);

            if (pieceType == PieceType::pawn && (endY == 0 || endY == 7)) {
                promotePawn(endX, endY, pieceColor);
            }

            switchTurn();
        }
    }

    
    void promotePawn(int x, int y, PieceColor pieceColor) {
        getTile(x, y).removePiece();
        getTile(x, y).setPiece(PieceType::queen, pieceColor);
    }
    int getSize() const { return size; }
private:
    GameState gameState{};
    const int size{};
    std::vector<std::vector<Tile>> board {};
};

std::map<std::string, Texture2D> pieceTextures;


void loadTextures() {
    pieceTextures["pawn_white"] = LoadTexture("images/pawn_white.png");
    pieceTextures["pawn_black"] = LoadTexture("images/pawn_black.png");
    pieceTextures["knight_white"] = LoadTexture("images/knight_white.png");
    pieceTextures["knight_black"] = LoadTexture("images/knight_black.png");
    pieceTextures["rook_white"] = LoadTexture("images/rook_white.png");
    pieceTextures["rook_black"] = LoadTexture("images/rook_black.png");
    pieceTextures["bishop_white"] = LoadTexture("images/bishop_white.png");
    pieceTextures["bishop_black"] = LoadTexture("images/bishop_black.png");
    pieceTextures["queen_white"] = LoadTexture("images/queen_white.png");
    pieceTextures["queen_black"] = LoadTexture("images/queen_black.png");
    pieceTextures["king_white"] = LoadTexture("images/king_white.png");
    pieceTextures["king_black"] = LoadTexture("images/king_black.png");
}


void unloadTextures() {
    for (auto& [key, texture] : pieceTextures) {
        UnloadTexture(texture);
    }
}


void drawBoard(Board& board, const std::vector<std::pair<int, int>>& validMoves,
    int selectedX, int selectedY, bool pieceSelected) {
    const int tileSize = 80;
    const int boardSize = board.getSize();

    for (int row = 0; row < boardSize; ++row) {
        for (int col = 0; col < boardSize; ++col) {
            
            if (pieceSelected && row == selectedY && col == selectedX) {
                DrawRectangle(col * tileSize, row * tileSize, tileSize, tileSize, GREEN);
                DrawRectangleLines(col * tileSize, row * tileSize, tileSize, tileSize, BLACK);
            }
            
            else if (std::find(validMoves.begin(), validMoves.end(), std::make_pair(col, row)) != validMoves.end()) {
                DrawRectangle(col * tileSize, row * tileSize, tileSize, tileSize, YELLOW);
                DrawRectangleLines(col * tileSize, row * tileSize, tileSize, tileSize, BLACK);
            }
            
            else {
                Color tileColor = (row + col) % 2 == 0 ? RAYWHITE : DARKGRAY;
                DrawRectangle(col * tileSize, row * tileSize, tileSize, tileSize, tileColor);
                DrawRectangleLines(col * tileSize, row * tileSize, tileSize, tileSize, BLACK);
            }

            
            Tile& tile = board.getTile(col, row);
            if (tile.hasPiece()) {
                const Piece& piece = *tile.getPiece();
                std::string textureKey;

                switch (piece.getType()) {
                case PieceType::pawn: textureKey = piece.getColor() == PieceColor::white ? "pawn_white" : "pawn_black"; break;
                case PieceType::knight: textureKey = piece.getColor() == PieceColor::white ? "knight_white" : "knight_black"; break;
                case PieceType::rook: textureKey = piece.getColor() == PieceColor::white ? "rook_white" : "rook_black"; break;
                case PieceType::bishop: textureKey = piece.getColor() == PieceColor::white ? "bishop_white" : "bishop_black"; break;
                case PieceType::queen: textureKey = piece.getColor() == PieceColor::white ? "queen_white" : "queen_black"; break;
                case PieceType::king: textureKey = piece.getColor() == PieceColor::white ? "king_white" : "king_black"; break;
                default: textureKey = ""; break;
                }

                if (!textureKey.empty()) {
                    Texture2D texture = pieceTextures[textureKey];
                    DrawTexture(texture, col * tileSize, row * tileSize, WHITE);
                }
            }
        }
    }
}


void handlePlayerInput(Board& board, PieceColor currentTurn,
    std::vector<std::pair<int, int>>& validMoves,
    int& selectedX, int& selectedY, bool& pieceSelected) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int tileX = GetMouseX() / 80; 
        int tileY = GetMouseY() / 80;

        if (!pieceSelected) {
            
            if (board.getTile(tileX, tileY).hasPiece() &&
                board.getTile(tileX, tileY).getPiece()->getColor() == currentTurn) {
                pieceSelected = true;
                selectedX = tileX;
                selectedY = tileY;

                
                validMoves.clear();
                for (int y = 0; y < board.getSize(); ++y) {
                    for (int x = 0; x < board.getSize(); ++x) {
                        if (board.validate(selectedX, selectedY, x, y,
                            currentTurn,
                            board.getTile(selectedX, selectedY).getPiece()->getType())) {
                            validMoves.emplace_back(x, y);
                        }
                    }
                }
            }
        }
        else {

            if (std::find(validMoves.begin(), validMoves.end(), std::make_pair(tileX, tileY)) != validMoves.end()) {
                board.makeMove(selectedX, selectedY, tileX, tileY,
                    board.getTile(selectedX, selectedY).getPiece()->getColor(),
                    board.getTile(selectedX, selectedY).getPiece()->getType());
            }
            pieceSelected = false;
            validMoves.clear();
        }
    }
}



int main()
{
    const int screenWidth = 640;
    const int screenHeight = 640;

    InitWindow(screenWidth, screenHeight, "Chess Game");

    Board chessBoard;
    loadTextures();

    bool pieceSelected = false;
    int selectedX = -1, selectedY = -1;
    std::vector<std::pair<int, int>> validMoves;

    chessBoard.placePiece(0, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(1, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(2, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(3, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(4, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(5, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(6, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(7, 1, PieceType::pawn, PieceColor::white);
    chessBoard.placePiece(7, 0, PieceType::rook, PieceColor::white);
    chessBoard.placePiece(0, 0, PieceType::rook, PieceColor::white);
    chessBoard.placePiece(6, 0, PieceType::knight, PieceColor::white);
    chessBoard.placePiece(1, 0, PieceType::knight, PieceColor::white);
    chessBoard.placePiece(5, 0, PieceType::bishop, PieceColor::white);
    chessBoard.placePiece(2, 0, PieceType::bishop, PieceColor::white);
    chessBoard.placePiece(4, 0, PieceType::queen, PieceColor::white);
    chessBoard.placePiece(3, 0, PieceType::king, PieceColor::white);

    chessBoard.placePiece(0, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(1, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(2, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(3, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(4, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(5, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(6, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(7, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(7, 7, PieceType::rook, PieceColor::black);
    chessBoard.placePiece(0, 7, PieceType::rook, PieceColor::black);
    chessBoard.placePiece(6, 7, PieceType::knight, PieceColor::black);
    chessBoard.placePiece(1, 7, PieceType::knight, PieceColor::black);
    chessBoard.placePiece(5, 7, PieceType::bishop, PieceColor::black);
    chessBoard.placePiece(2, 7, PieceType::bishop, PieceColor::black);
    chessBoard.placePiece(4, 7, PieceType::queen, PieceColor::black);
    chessBoard.placePiece(3, 7, PieceType::king, PieceColor::black);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
       
        PieceColor currentTurn = chessBoard.isTurnValid(PieceColor::white) ? PieceColor::white : PieceColor::black;

        handlePlayerInput(chessBoard, currentTurn, validMoves, selectedX, selectedY, pieceSelected);

        BeginDrawing();
        ClearBackground(BLACK);

        drawBoard(chessBoard, validMoves, selectedX, selectedY, pieceSelected);

        EndDrawing();
    }

    unloadTextures();

    CloseWindow();

    return 0;
}

