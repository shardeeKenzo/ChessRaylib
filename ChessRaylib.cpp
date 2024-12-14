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

bool isAnimating = false;              // Whether an animation is ongoing
float animationTime = 0.0f;            // Elapsed time for the current animation
float animationDuration = 0.3f;        // Total duration of the animation in seconds
int animStartX = -1, animStartY = -1;  // Starting tile of the animated piece
int animEndX = -1, animEndY = -1;      // Ending tile of the animated piece
Piece animatingPiece;                  // Piece being animated

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
public:
    Board(const int size = 8)
        : size(size), gameState{ GameState::whiteTurn }, lastDoubleMove{ -1, -1 },
        whiteKingMoved{ false }, blackKingMoved{ false },
        whiteRookMovedLeft{ false }, whiteRookMovedRight{ false },
        blackRookMovedLeft{ false }, blackRookMovedRight{ false } {
        for (int y = 0; y < size; ++y) {
            std::vector<Tile> row;
            for (int x = 0; x < size; ++x) {
                row.emplace_back(Tile(x, y));
            }
            board.push_back(row);
        }
    }

    bool isKingInCheck(PieceColor pieceColor) {
        std::pair<int, int> kingPosition = findKing(pieceColor);
        return isTileUnderAttack(kingPosition.first, kingPosition.second, pieceColor);
    }

    bool isKingInCheckmate(PieceColor pieceColor) {
        if (!isKingInCheck(pieceColor)) return false;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                Tile& tile = getTile(x, y);
                if (tile.hasPiece() && tile.getPiece()->getColor() == pieceColor) {
                    for (int targetY = 0; targetY < size; ++targetY) {
                        for (int targetX = 0; targetX < size; ++targetX) {
                            if (validate(x, y, targetX, targetY, pieceColor, tile.getPiece()->getType())) {
                                Piece capturedPiece = simulateMove(x, y, targetX, targetY);
                                bool stillInCheck = isKingInCheck(pieceColor);
                                undoMove(x, y, targetX, targetY, capturedPiece);
                                if (!stillInCheck) return false;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

    bool isStalemate(PieceColor pieceColor) {
        if (isKingInCheck(pieceColor)) return false;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                Tile& tile = getTile(x, y);
                if (tile.hasPiece() && tile.getPiece()->getColor() == pieceColor) {
                    for (int targetY = 0; targetY < size; ++targetY) {
                        for (int targetX = 0; targetX < size; ++targetX) {
                            if (validate(x, y, targetX, targetY, pieceColor, tile.getPiece()->getType())) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

    bool isTurnValid(PieceColor pieceColor) const {
        return (gameState == GameState::whiteTurn && pieceColor == PieceColor::white) ||
               (gameState == GameState::blackTurn && pieceColor == PieceColor::black);
    }

    void switchTurn() {
        gameState = (gameState == GameState::whiteTurn) ? GameState::blackTurn : GameState::whiteTurn;
    }

    Tile& getTile(int x, int y) {
        if (x < 0 || x >= size || y < 0 || y >= size) {
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
        if (!isMoveValid(startX, startY, endX, endY, pieceColor, pieceType)) {
            return false;
        }

        Piece capturedPiece = simulateMove(startX, startY, endX, endY);

        bool stillInCheck = isKingInCheck(pieceColor);

        undoMove(startX, startY, endX, endY, capturedPiece);

        return !stillInCheck;
    }
    
    void promotePawn(int x, int y, PieceColor pieceColor) {
        getTile(x, y).removePiece();
        getTile(x, y).setPiece(PieceType::queen, pieceColor);
    }

    bool isCastlingValid(int startX, int startY, int endX, int endY, PieceColor pieceColor) {

        if (abs(endX - startX) != 2 || startY != endY) return false;
        int rookX = (endX > startX) ? 7 : 0;
        Tile& rookTile = getTile(rookX, startY);

        if (!rookTile.hasPiece() || rookTile.getPiece()->getType() != PieceType::rook) return false;

        if (pieceColor == white) {
            if (whiteKingMoved || (rookX == 0 && whiteRookMovedLeft) || (rookX == 7 && whiteRookMovedRight)) {
                return false;
            }
        }
        else if (pieceColor == black) {
            if (blackKingMoved || (rookX == 0 && blackRookMovedLeft) || (rookX == 7 && blackRookMovedRight)) {
                return false;
            }
        }

        if (!isPathClear(startX, startY, rookX, startY)) return false;

        return true;
    }

    bool isEnPassantValid(int startX, int startY, int endX, int endY, PieceColor pieceColor) {
        if (abs(endX - startX) == 1 && endY - startY == (pieceColor == PieceColor::white ? 1 : -1)) {
            return lastDoubleMove.first == endX && lastDoubleMove.second == startY;
        }
        return false;
    }

    bool isMoveValid(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        if (endX < 0 || endX >= size || endY < 0 || endY >= size) {
            return false;
        }

        if (startX == endX && startY == endY) {
            return false;
        }

        if (getTile(endX, endY).hasPiece() &&
            getTile(endX, endY).getPiece()->getColor() == pieceColor) {
            return false;
        }

        int deltaX = abs(endX - startX);
        int deltaY = abs(endY - startY);

        switch (pieceType) {
        case pawn:
            if (isEnPassantValid(startX, startY, endX, endY, pieceColor)) return true;

            if (pieceColor == PieceColor::white) {
                if (startY == 1 && endY - startY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) {
                    lastDoubleMove = { endX, endY };
                    return true;
                }
                if (endY - startY == 1 && deltaX == 0 && !getTile(endX, endY).hasPiece()) return true;
                if (endY - startY == 1 && deltaX == 1 && getTile(endX, endY).hasPiece() &&
                    getTile(endX, endY).getPiece()->getColor() == PieceColor::black) return true;
            }
            else if (pieceColor == PieceColor::black) {
                if (startY == 6 && startY - endY == 2 && deltaX == 0 && isPathClear(startX, startY, endX, endY)) {
                    lastDoubleMove = { endX, endY };
                    return true;
                }
                if (startY - endY == 1 && deltaX == 0 && !getTile(endX, endY).hasPiece()) return true;
                if (startY - endY == 1 && deltaX == 1 && getTile(endX, endY).hasPiece() &&
                    getTile(endX, endY).getPiece()->getColor() == PieceColor::white) return true;
            }
            return false;

        case king:
            if (isCastlingValid(startX, startY, endX, endY, pieceColor)) return true;
            return deltaX <= 1 && deltaY <= 1;

        case knight:
            return (deltaX == 2 && deltaY == 1) || (deltaX == 1 && deltaY == 2);

        case bishop:
            return deltaX == deltaY && isPathClear(startX, startY, endX, endY);

        case rook:
            return ((deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0)) && isPathClear(startX, startY, endX, endY);

        case queen:
            return ((deltaX == deltaY) || (deltaX == 0 && deltaY > 0) || (deltaY == 0 && deltaX > 0)) && isPathClear(startX, startY, endX, endY);

        default:
            return false;
        }
    }

    void makeMove(int startX, int startY, int endX, int endY, PieceColor pieceColor, PieceType pieceType) {
        if (isCastlingValid(startX, startY, endX, endY, pieceColor)) {
            
            int rookStartX = (endX > startX) ? 7 : 0;  
            int rookEndX = (endX > startX) ? endX - 1 : endX + 1;

            Piece rook = *getTile(rookStartX, startY).getPiece();
            getTile(rookStartX, startY).removePiece();
            getTile(rookEndX, startY).setPiece(PieceType::rook, pieceColor);

            getTile(startX, startY).removePiece();
            getTile(endX, endY).setPiece(pieceType, pieceColor);

            if (pieceColor == PieceColor::white) {
                whiteKingMoved = true;
                if (rookStartX == 0) whiteRookMovedLeft = true;
                if (rookStartX == 7) whiteRookMovedRight = true;
            }
            else if (pieceColor == PieceColor::black) {
                blackKingMoved = true;
                if (rookStartX == 0) blackRookMovedLeft = true;
                if (rookStartX == 7) blackRookMovedRight = true;
            }

            switchTurn();
            return;
        }

        if (isEnPassantValid(startX, startY, endX, endY, pieceColor)) {
          
            int capturedPawnY = (pieceColor == PieceColor::white) ? endY - 1 : endY + 1;
            getTile(endX, capturedPawnY).removePiece();
        }
        getTile(startX, startY).removePiece();
        getTile(endX, endY).setPiece(pieceType, pieceColor);

        if (pieceType == PieceType::pawn && (endY == 0 || endY == 7)) {
            promotePawn(endX, endY, pieceColor);
        }

        if (pieceType == PieceType::king) {
            if (pieceColor == PieceColor::white) whiteKingMoved = true;
            if (pieceColor == PieceColor::black) blackKingMoved = true;
        }

        if (pieceType == PieceType::rook) {
            if (pieceColor == PieceColor::white) {
                if (startX == 0 && startY == 0) whiteRookMovedLeft = true;
                if (startX == 7 && startY == 0) whiteRookMovedRight = true;
            }
            else if (pieceColor == PieceColor::black) {
                if (startX == 0 && startY == 7) blackRookMovedLeft = true;
                if (startX == 7 && startY == 7) blackRookMovedRight = true; 
            }
        }

        switchTurn();
    }

    int getSize() const { return size; }
private:
    GameState gameState;
    const int size;
    std::vector<std::vector<Tile>> board;
    std::pair<int, int> lastDoubleMove; 
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookMovedLeft;
    bool whiteRookMovedRight;
    bool blackRookMovedLeft;
    bool blackRookMovedRight;

    std::pair<int, int> findKing(PieceColor pieceColor) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                Tile& tile = getTile(x, y);
                if (tile.hasPiece() && tile.getPiece()->getType() == PieceType::king &&
                    tile.getPiece()->getColor() == pieceColor) {
                    return { x, y };
                }
            }
        }
        throw std::runtime_error("King not found on the board!");
    }

    bool isTileUnderAttack(int x, int y, PieceColor defenderColor) {
        PieceColor attackerColor = (defenderColor == PieceColor::white) ? PieceColor::black : PieceColor::white;

        for (int row = 0; row < size; ++row) {
            for (int col = 0; col < size; ++col) {
                Tile& tile = getTile(col, row);
                if (tile.hasPiece() && tile.getPiece()->getColor() == attackerColor &&
                    isMoveValid(col, row, x, y, attackerColor, tile.getPiece()->getType())) {
                    return true;
                }
            }
        }
        return false;
    }

    Piece simulateMove(int startX, int startY, int endX, int endY) {
        Piece capturedPiece = Piece();
        if (getTile(endX, endY).hasPiece()) {
            capturedPiece = *getTile(endX, endY).getPiece();
        }

        getTile(endX, endY).setPiece(getTile(startX, startY).getPiece()->getType(),
            getTile(startX, startY).getPiece()->getColor());
        getTile(startX, startY).removePiece();

        return capturedPiece;
    }

    void undoMove(int startX, int startY, int endX, int endY, Piece capturedPiece) {
        getTile(startX, startY).setPiece(getTile(endX, endY).getPiece()->getType(),
            getTile(endX, endY).getPiece()->getColor());
        getTile(endX, endY).removePiece();

        if (capturedPiece.getType() != PieceType::none) {
            getTile(endX, endY).setPiece(capturedPiece.getType(), capturedPiece.getColor());
        }
    }
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

void updateAnimation(Board& board, float deltaTime) {
    if (!isAnimating) return;

    animationTime += deltaTime;
    if (animationTime >= animationDuration) {
        isAnimating = false;
        board.makeMove(animStartX, animStartY, animEndX, animEndY, animatingPiece.getColor(), animatingPiece.getType());
    }
}

float Lerp(float start, float end, float t) {
    return start + t * (end - start);
}


void drawBoard(Board& board, const std::vector<std::pair<int, int>>& validMoves,
    int selectedX, int selectedY, bool pieceSelected) {
    const int tileSize = 80;
    const int boardSize = board.getSize();
    const int margin = 20;

    for (int col = 0; col < boardSize; ++col) {
        std::string label(1, 'A' + col);
        DrawText(label.c_str(), margin + col * tileSize + tileSize / 2 - 5, 0, 20, WHITE);
        DrawText(label.c_str(), margin + col * tileSize + tileSize / 2 - 5, margin + boardSize * tileSize + 5, 20, WHITE);
    }

    for (int row = 0; row < boardSize; ++row) {
        std::string label = std::to_string(boardSize - row);
        DrawText(label.c_str(), 0, margin + row * tileSize + tileSize / 2 - 10, 20, WHITE);
        DrawText(label.c_str(), margin + boardSize * tileSize + 5, margin + row * tileSize + tileSize / 2 - 10, 20, WHITE);
    }

    for (int row = 0; row < boardSize; ++row) {
        for (int col = 0; col < boardSize; ++col) {
            Color tileColor = (row + col) % 2 == 0 ? RAYWHITE : DARKGRAY;
            DrawRectangle(margin + col * tileSize, margin + row * tileSize, tileSize, tileSize, tileColor);
            DrawRectangleLines(margin + col * tileSize, margin + row * tileSize, tileSize, tileSize, BLACK);

            if (pieceSelected && row == selectedY && col == selectedX) {
                DrawRectangle(margin + col * tileSize, margin + row * tileSize, tileSize, tileSize, GREEN);
            }
            else if (std::find(validMoves.begin(), validMoves.end(), std::make_pair(col, row)) != validMoves.end()) {
                DrawRectangle(margin + col * tileSize, margin + row * tileSize, tileSize, tileSize, YELLOW);
            }

            Tile& tile = board.getTile(col, row);
            if (tile.hasPiece() && (!isAnimating || animStartX != col || animStartY != row)) {
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
                    DrawTexture(texture, margin + col * tileSize, margin + row * tileSize, WHITE);
                }
            }
        }
    }

    if (isAnimating) {
        float t = animationTime / animationDuration;
        float animX = Lerp(animStartX * tileSize, animEndX * tileSize, t);
        float animY = Lerp(animStartY * tileSize, animEndY * tileSize, t);

        std::string textureKey;
        switch (animatingPiece.getType()) {
        case PieceType::pawn: textureKey = animatingPiece.getColor() == PieceColor::white ? "pawn_white" : "pawn_black"; break;
        case PieceType::knight: textureKey = animatingPiece.getColor() == PieceColor::white ? "knight_white" : "knight_black"; break;
        case PieceType::rook: textureKey = animatingPiece.getColor() == PieceColor::white ? "rook_white" : "rook_black"; break;
        case PieceType::bishop: textureKey = animatingPiece.getColor() == PieceColor::white ? "bishop_white" : "bishop_black"; break;
        case PieceType::queen: textureKey = animatingPiece.getColor() == PieceColor::white ? "queen_white" : "queen_black"; break;
        case PieceType::king: textureKey = animatingPiece.getColor() == PieceColor::white ? "king_white" : "king_black"; break;
        default: textureKey = ""; break;
        }

        if (!textureKey.empty()) {
            Texture2D texture = pieceTextures[textureKey];
            DrawTexture(texture, margin + animX, margin + animY, WHITE);
        }

        if (animatingPiece.getType() == PieceType::king && abs(animEndX - animStartX) == 2) {
            int rookStartX = (animEndX > animStartX) ? 7 : 0;
            int rookEndX = (animEndX > animStartX) ? animEndX - 1 : animEndX + 1;

            float rookAnimX = Lerp(rookStartX * tileSize, rookEndX * tileSize, t);
            float rookAnimY = animStartY * tileSize;

            std::string rookTextureKey = animatingPiece.getColor() == PieceColor::white ? "rook_white" : "rook_black";
            Texture2D rookTexture = pieceTextures[rookTextureKey];
            DrawTexture(rookTexture, rookAnimX, rookAnimY, WHITE);
        }
    }
}


void handlePlayerInput(Board& board, PieceColor currentTurn,
    std::vector<std::pair<int, int>>& validMoves,
    int& selectedX, int& selectedY, bool& pieceSelected) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int tileX = GetMouseX() / 80;
        int tileY = GetMouseY() / 80;

        if (tileX < 0 || tileX >= board.getSize() || tileY < 0 || tileY >= board.getSize()) {
            return;
        }

        if (!pieceSelected) {
            if (board.getTile(tileX, tileY).hasPiece() &&
                board.getTile(tileX, tileY).getPiece()->getColor() == currentTurn) {
                pieceSelected = true;
                selectedX = tileX;
                selectedY = tileY;

                // Generate valid moves
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
            else {
                return;
            }
        }
        else {
            if (std::find(validMoves.begin(), validMoves.end(), std::make_pair(tileX, tileY)) != validMoves.end()) {
                isAnimating = true;
                animationTime = 0.0f;
                animStartX = selectedX;
                animStartY = selectedY;
                animEndX = tileX;
                animEndY = tileY;
                animatingPiece = *board.getTile(selectedX, selectedY).getPiece();

                pieceSelected = false;
                validMoves.clear();
            }
            else {
                pieceSelected = false;
                validMoves.clear();
            }
        }
    }
}

int main()
{
    const int screenWidth = 640 + 2 * 20;
    const int screenHeight = 640 + 2 * 20;

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
        float deltaTime = GetFrameTime();

        if (isAnimating) {
            updateAnimation(chessBoard, deltaTime);
        }
        else {
            PieceColor currentTurn = chessBoard.isTurnValid(PieceColor::white) ? PieceColor::white : PieceColor::black;

            if (chessBoard.isKingInCheckmate(currentTurn)) {
                DrawText("Checkmate! Game Over.", 100, 100, 20, RED);
            }
            else if (chessBoard.isStalemate(currentTurn)) {
                DrawText("Stalemate! Game Draw.", 100, 100, 20, YELLOW);
            }
            else {
                if (chessBoard.isKingInCheck(currentTurn)) {
                    DrawText("Check!", 100, 100, 20, ORANGE);
                }

                handlePlayerInput(chessBoard, currentTurn, validMoves, selectedX, selectedY, pieceSelected);
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);
        drawBoard(chessBoard, validMoves, selectedX, selectedY, pieceSelected);
        EndDrawing();
    }

    unloadTextures();

    CloseWindow();

    return 0;
}

