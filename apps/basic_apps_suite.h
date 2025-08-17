#ifndef BASIC_APPS_SUITE_H
#define BASIC_APPS_SUITE_H

#include "base_app.h"
#include <vector>
#include <string>
#include <map>

// Expense tracking structures
struct Expense {
    uint32_t id;
    std::string description;
    float amount;
    std::string category;
    time_t date;
    bool isIncome;
};

struct Budget {
    std::string category;
    float budgetAmount;
    float spentAmount;
};

// Simple spreadsheet structures
struct Cell {
    std::string value;
    std::string formula;
    int row;
    int col;
};

// Game states
enum class GameType {
    NONE,
    CALCULATOR,
    MEMORY_GAME,
    SIMPLE_PUZZLE
};

enum class MemoryGameState {
    SHOWING,
    HIDDEN,
    COMPLETE
};

class BasicAppsSuite : public BaseApp {
public:
    BasicAppsSuite();
    ~BasicAppsSuite() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void createTabView();
    void createExpenseTab();
    void createSpreadsheetTab();
    void createGamesTab();
    void createCalculatorTab();
    
    // Expense tracking functions
    void createExpenseList();
    void createExpenseSummary();
    void createAddExpenseDialog();
    void showAddExpenseDialog(Expense* expense = nullptr);
    void hideAddExpenseDialog();
    void addExpense(const Expense& expense);
    void editExpense(uint32_t expenseId, const Expense& expense);
    void deleteExpense(uint32_t expenseId);
    void updateExpenseSummary();
    void filterExpensesByCategory(const std::string& category);
    void filterExpensesByDateRange(time_t startDate, time_t endDate);
    
    // Spreadsheet functions
    void createSpreadsheetGrid();
    void createSpreadsheetToolbar();
    void updateCell(int row, int col, const std::string& value);
    void calculateFormula(int row, int col);
    void selectCell(int row, int col);
    void copyCell();
    void pasteCell();
    std::string getCellReference(int row, int col);
    float evaluateExpression(const std::string& expression);
    
    // Games functions
    void createMemoryGame();
    void createSimplePuzzle();
    void startMemoryGame();
    void resetMemoryGame();
    void flipMemoryCard(int index);
    void checkMemoryGameComplete();
    
    void startSimplePuzzle();
    void resetSimplePuzzle();
    void movePuzzlePiece(int index);
    void checkPuzzleComplete();
    
    // Calculator functions
    void createCalculator();
    void appendToCalculator(const std::string& value);
    void calculateResult();
    void clearCalculator();
    
    // Utility functions
    std::string formatCurrency(float amount);
    std::string formatDate(time_t date);
    float parseFloat(const std::string& str);
    
    os_error_t saveExpenses();
    os_error_t loadExpenses();
    os_error_t saveSpreadsheet();
    os_error_t loadSpreadsheet();
    
    // Event callbacks
    static void tabChangedCallback(lv_event_t* e);
    static void addExpenseCallback(lv_event_t* e);
    static void saveExpenseCallback(lv_event_t* e);
    static void cancelExpenseCallback(lv_event_t* e);
    static void expenseListCallback(lv_event_t* e);
    static void categoryFilterCallback(lv_event_t* e);
    static void cellClickCallback(lv_event_t* e);
    static void cellEditCallback(lv_event_t* e);
    static void copyButtonCallback(lv_event_t* e);
    static void pasteButtonCallback(lv_event_t* e);
    static void memoryCardCallback(lv_event_t* e);
    static void puzzlePieceCallback(lv_event_t* e);
    static void gameResetCallback(lv_event_t* e);
    static void calculatorButtonCallback(lv_event_t* e);
    
    // Data
    std::vector<Expense> m_expenses;
    std::vector<Budget> m_budgets;
    std::map<std::pair<int, int>, Cell> m_spreadsheetCells;
    uint32_t m_nextExpenseId;
    
    // Expense tracking state
    float m_totalIncome;
    float m_totalExpenses;
    float m_balance;
    std::string m_selectedCategory;
    
    // Spreadsheet state
    int m_selectedRow;
    int m_selectedCol;
    Cell m_copiedCell;
    bool m_hasCopiedCell;
    
    // Games state
    GameType m_currentGame;
    std::vector<int> m_memorySequence;
    std::vector<bool> m_memoryCardFlipped;
    std::vector<int> m_memoryCardValues;
    int m_memoryGameLevel;
    MemoryGameState m_memoryGameState;
    
    std::vector<int> m_puzzlePieces;
    int m_puzzleEmptyIndex;
    int m_puzzleSize;
    
    // Calculator state
    std::string m_calculatorExpression;
    std::string m_calculatorResult;
    bool m_calculatorNewInput;
    
    // UI elements
    lv_obj_t* m_tabView = nullptr;
    lv_obj_t* m_expenseTab = nullptr;
    lv_obj_t* m_spreadsheetTab = nullptr;
    lv_obj_t* m_gamesTab = nullptr;
    lv_obj_t* m_calculatorTab = nullptr;
    
    // Expense UI
    lv_obj_t* m_expenseList = nullptr;
    lv_obj_t* m_expenseSummary = nullptr;
    lv_obj_t* m_addExpenseDialog = nullptr;
    lv_obj_t* m_expenseDescInput = nullptr;
    lv_obj_t* m_expenseAmountInput = nullptr;
    lv_obj_t* m_expenseCategoryDropdown = nullptr;
    lv_obj_t* m_expenseIncomeSwitch = nullptr;
    lv_obj_t* m_categoryFilterDropdown = nullptr;
    
    // Spreadsheet UI
    lv_obj_t* m_spreadsheetGrid = nullptr;
    lv_obj_t* m_spreadsheetToolbar = nullptr;
    lv_obj_t* m_cellEditor = nullptr;
    lv_obj_t* m_cellReference = nullptr;
    
    // Games UI
    lv_obj_t* m_gameContainer = nullptr;
    lv_obj_t* m_memoryGameGrid = nullptr;
    lv_obj_t* m_puzzleGrid = nullptr;
    lv_obj_t* m_gameScoreLabel = nullptr;
    lv_obj_t* m_gameStatusLabel = nullptr;
    
    // Calculator UI
    lv_obj_t* m_calculatorDisplay = nullptr;
    lv_obj_t* m_calculatorGrid = nullptr;
    
    Expense* m_editingExpense = nullptr;
};

#endif // BASIC_APPS_SUITE_H