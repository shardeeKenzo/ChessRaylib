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
    unknown,
    black,
    white
};

class Piece {
public:
    Piece() : color{ PieceColor::unknown }, pieceType{ PieceType::none }
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

bool isMoveValid(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {

    switch (pieceType) {
        // Calculate deltas
        int deltaX = abs(endX - startX);
        int deltaY = abs(endY - startY);

        case pawn:
            // Pawns move 1 square forward (2 squares on first move) and capture diagonally
            if (pieceColor == PieceColor::white) {
                // Forward movement
                if (startY == 1 && endY - startY == 2 && deltaX == 0) return true; // First move
                if (endY - startY == 1 && deltaX == 0) return true;               // Normal move
                // Capture movement
                if (endY - startY == 1 && deltaX == 1) return true;
            }
            else if (pieceColor == PieceColor::black) {
                // Forward movement
                if (startY == 6 && startY - endY == 2 && deltaX == 0) return true; // First move
                if (startY - endY == 1 && deltaX == 0) return true;               // Normal move
                // Capture movement
                if (startY - endY == 1 && deltaX == 1) return true;
            }
            return false;

        case knight:
            // Knight moves in an "L" shape
            return (deltaX == 2 && deltaY == 1) || (deltaX == 1 && deltaY == 2);

        case bishop:
            // Bishop moves diagonally
            return deltaX == deltaY;

        case rook:
            // Rook moves horizontally or vertically
            return (deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0);

        case queen:
            // Queen combines bishop and rook moves
            return (deltaX == deltaY) || (deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0);

        case king:
            // King moves 1 square in any direction
            return deltaX <= 1 && deltaY <= 1;

        default:
            return false;
    }
}

class Board {
public:
    Board(const int size = 8) : size(size){
        for (int y = 0; y < size; ++y) {
            std::vector<Tile> row;
            for (int x = 0; x < size; ++x) {
                row.emplace_back(Tile(x, y));
            }
            board.push_back(row);
        }
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
    void makeMove(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        if (isMoveValid(startX, startY, endX, endY, pieceColor, pieceType)) {
            getTile(startX, startY).removePiece();
            getTile(endX, endY).setPiece(pieceType, pieceColor);
        }
    }
    int getSize() const { return size; }
private:
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

    chessBoard.placePiece(0, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(1, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(2, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(3, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(4, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(5, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(6, 6, PieceType::pawn, PieceColor::black);
    chessBoard.placePiece(7, 6, PieceType::pawn, PieceColor::black);

    chessBoard.makeMove(0, 1, 0, 2, white, pawn);
    chessBoard.makeMove(7, 6, 7, 5, black, pawn);

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

