#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <cmath>
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
                if (startY == 1 && endY - startY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) return true; // First move
                if (endY - startY == 1 && deltaX == 0) return true; // Normal move
                if (endY - startY == 1 && deltaX == 1) return true; // Capture
            }
            else if (pieceColor == PieceColor::black) {
                if (startY == 6 && startY - endY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) return true; // First move
                if (startY - endY == 1 && deltaX == 0) return true; // Normal move
                if (startY - endY == 1 && deltaX == 1) return true; // Capture
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
            switchTurn();
        }
    }

    // pawn promotion, add where its needed
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


void drawBoard(Board& board) {
    const int tileSize = 80;
    const int boardSize = board.getSize();

    for (int row = 0; row < boardSize; ++row) {
        for (int col = 0; col < boardSize; ++col) {
            Color tileColor = (row + col) % 2 == 0 ? RAYWHITE : DARKGRAY;
            DrawRectangle(col * tileSize, row * tileSize, tileSize, tileSize, tileColor);

            Tile& tile = board.getTile(col, row);

            if (tile.hasPiece()) {
                const Piece& piece = *tile.getPiece();
                std::string pieceSymbol{};

                switch (piece.getType()) {
                case PieceType::pawn: pieceSymbol = "P"; break;
                case PieceType::knight: pieceSymbol = "N"; break;
                case PieceType::bishop: pieceSymbol = "B"; break;
                case PieceType::rook: pieceSymbol = "R"; break;
                case PieceType::queen: pieceSymbol = "Q"; break;
                case PieceType::king: pieceSymbol = "K"; break;
                default: pieceSymbol = ""; break;
                }

                if (!pieceSymbol.empty()) {
                    DrawText(pieceSymbol.c_str(),
                        col * tileSize + tileSize / 2 - 10,
                        row * tileSize + tileSize / 2 - 10,
                        20,
                        piece.getColor() == PieceColor::white ? BLUE : RED);
                }
            }
        }
    }

}


int main()
{
    const int screenWidth = 640;
    const int screenHeight = 640;

    InitWindow(screenWidth, screenHeight, "Chess Game");

    Board chessBoard;

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

    chessBoard.makeMove(0, 1, 0, 3, white, pawn);
    chessBoard.makeMove(7, 6, 7, 4, black, pawn);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        drawBoard(chessBoard);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

