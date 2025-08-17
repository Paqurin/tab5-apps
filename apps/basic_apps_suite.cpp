#include "basic_apps_suite.h"
#include "../system/os_manager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

BasicAppsSuite::BasicAppsSuite() 
    : BaseApp("com.m5stack.basicapps", "Basic Apps", "1.0.0"), 
      m_nextExpenseId(1), m_totalIncome(0), m_totalExpenses(0), m_balance(0),
      m_selectedRow(0), m_selectedCol(0), m_hasCopiedCell(false),
      m_currentGame(GameType::NONE), m_memoryGameLevel(1), 
      m_memoryGameState(MemoryGameState::HIDDEN), m_puzzleSize(3), m_puzzleEmptyIndex(8),
      m_calculatorNewInput(true) {
    setDescription("Basic applications suite with expense tracking, games, and spreadsheet");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

BasicAppsSuite::~BasicAppsSuite() {
    shutdown();
}

os_error_t BasicAppsSuite::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Basic Apps Suite");
    
    // Initialize data
    loadExpenses();
    loadSpreadsheet();
    
    // Initialize puzzle
    for (int i = 0; i < 8; i++) {
        m_puzzlePieces.push_back(i + 1);
    }
    m_puzzlePieces.push_back(0); // Empty space
    
    // Set memory usage estimate
    setMemoryUsage(50240); // 49KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t BasicAppsSuite::update(uint32_t deltaTime) {
    // Update games if needed
    if (m_currentGame == GameType::MEMORY_GAME && m_memoryGameState == MemoryGameState::SHOWING) {
        // Could add timer logic here for memory game
    }
    
    return OS_OK;
}

os_error_t BasicAppsSuite::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Basic Apps Suite");
    
    // Save data
    saveExpenses();
    saveSpreadsheet();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t BasicAppsSuite::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - 60 - 40); // Account for status bar and dock
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_uiContainer, 5, 0);

    createMainUI();
    return OS_OK;
}

os_error_t BasicAppsSuite::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void BasicAppsSuite::createMainUI() {
    createTabView();
}

void BasicAppsSuite::createTabView() {
    m_tabView = lv_tabview_create(m_uiContainer, LV_DIR_TOP, 50);
    lv_obj_set_size(m_tabView, LV_PCT(100), LV_PCT(100));
    
    // Create tabs
    m_expenseTab = lv_tabview_add_tab(m_tabView, "Expenses");
    m_calculatorTab = lv_tabview_add_tab(m_tabView, "Calculator");
    m_gamesTab = lv_tabview_add_tab(m_tabView, "Games");
    m_spreadsheetTab = lv_tabview_add_tab(m_tabView, "Spreadsheet");
    
    // Setup tab content
    createExpenseTab();
    createCalculatorTab();
    createGamesTab();
    createSpreadsheetTab();
    
    lv_obj_add_event_cb(m_tabView, tabChangedCallback, LV_EVENT_VALUE_CHANGED, this);
}

void BasicAppsSuite::createExpenseTab() {
    // Create expense summary at top
    m_expenseSummary = lv_obj_create(m_expenseTab);
    lv_obj_set_size(m_expenseSummary, LV_PCT(100), 80);
    lv_obj_align(m_expenseSummary, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_expenseSummary, lv_color_hex(0x2C2C2C), 0);
    
    // Create toolbar
    lv_obj_t* toolbar = lv_obj_create(m_expenseTab);
    lv_obj_set_size(toolbar, LV_PCT(100), 50);
    lv_obj_align_to(toolbar, m_expenseSummary, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(toolbar, lv_color_hex(0x2C2C2C), 0);
    
    // Add expense button
    lv_obj_t* addBtn = lv_btn_create(toolbar);
    lv_obj_set_size(addBtn, 100, LV_PCT(80));
    lv_obj_align(addBtn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(addBtn, addExpenseCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* addLabel = lv_label_create(addBtn);
    lv_label_set_text(addLabel, "Add Expense");
    lv_obj_center(addLabel);
    
    // Category filter
    m_categoryFilterDropdown = lv_dropdown_create(toolbar);
    lv_obj_set_size(m_categoryFilterDropdown, 120, LV_PCT(80));
    lv_obj_align(m_categoryFilterDropdown, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_dropdown_set_options(m_categoryFilterDropdown, "All\nFood\nTransport\nShopping\nEntertainment\nBills\nOther");
    lv_obj_add_event_cb(m_categoryFilterDropdown, categoryFilterCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Create expense list
    m_expenseList = lv_list_create(m_expenseTab);
    lv_obj_set_size(m_expenseList, LV_PCT(100), LV_PCT(100) - 140);
    lv_obj_align_to(m_expenseList, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_expenseList, lv_color_hex(0x2C2C2C), 0);
    
    updateExpenseSummary();
}

void BasicAppsSuite::createCalculatorTab() {
    // Create display
    m_calculatorDisplay = lv_textarea_create(m_calculatorTab);
    lv_obj_set_size(m_calculatorDisplay, LV_PCT(95), 60);
    lv_obj_align(m_calculatorDisplay, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_text(m_calculatorDisplay, "0");
    lv_textarea_set_one_line(m_calculatorDisplay, true);
    lv_obj_set_style_text_align(m_calculatorDisplay, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_add_state(m_calculatorDisplay, LV_STATE_DISABLED);
    
    // Create button grid
    m_calculatorGrid = lv_obj_create(m_calculatorTab);
    lv_obj_set_size(m_calculatorGrid, LV_PCT(95), LV_PCT(80));
    lv_obj_align_to(m_calculatorGrid, m_calculatorDisplay, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_layout(m_calculatorGrid, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_all(m_calculatorGrid, 5, 0);
    lv_obj_set_style_pad_gap(m_calculatorGrid, 5, 0);
    
    // Define grid template
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(m_calculatorGrid, col_dsc, row_dsc);
    
    // Button layout: 5 rows x 4 columns
    const char* buttons[5][4] = {
        {"C", "±", "%", "÷"},
        {"7", "8", "9", "×"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "=", "="}
    };
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
            lv_obj_t* btn = lv_btn_create(m_calculatorGrid);
            
            // Special layout for 0 and = buttons
            if (row == 4 && col == 0) {
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 2, LV_GRID_ALIGN_STRETCH, row, 1);
            } else if (row == 4 && col == 1) {
                continue; // Skip this position as 0 takes 2 columns
            } else if (row == 4 && col == 3) {
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
            } else {
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
            }
            
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, buttons[row][col]);
            lv_obj_center(label);
            
            // Set button colors
            if (col == 3 || strcmp(buttons[row][col], "=") == 0) {
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xE67E22), 0); // Orange for operators
            } else if (row == 0) {
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x95A5A6), 0); // Gray for functions
            } else {
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x34495E), 0); // Dark for numbers
            }
            
            lv_obj_set_user_data(btn, (void*)buttons[row][col]);
            lv_obj_add_event_cb(btn, calculatorButtonCallback, LV_EVENT_CLICKED, this);
        }
    }
}

void BasicAppsSuite::createGamesTab() {
    // Game selection buttons
    lv_obj_t* gameSelector = lv_obj_create(m_gamesTab);
    lv_obj_set_size(gameSelector, LV_PCT(100), 60);
    lv_obj_align(gameSelector, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(gameSelector, lv_color_hex(0x2C2C2C), 0);
    
    lv_obj_t* memoryBtn = lv_btn_create(gameSelector);
    lv_obj_set_size(memoryBtn, 120, 40);
    lv_obj_align(memoryBtn, LV_ALIGN_LEFT_MID, 10, 0);
    
    lv_obj_t* memoryLabel = lv_label_create(memoryBtn);
    lv_label_set_text(memoryLabel, "Memory Game");
    lv_obj_center(memoryLabel);
    
    lv_obj_t* puzzleBtn = lv_btn_create(gameSelector);
    lv_obj_set_size(puzzleBtn, 120, 40);
    lv_obj_align_to(puzzleBtn, memoryBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    lv_obj_t* puzzleLabel = lv_label_create(puzzleBtn);
    lv_label_set_text(puzzleLabel, "Puzzle Game");
    lv_obj_center(puzzleLabel);
    
    // Game container
    m_gameContainer = lv_obj_create(m_gamesTab);
    lv_obj_set_size(m_gameContainer, LV_PCT(100), LV_PCT(100) - 70);
    lv_obj_align_to(m_gameContainer, gameSelector, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_gameContainer, lv_color_hex(0x2C2C2C), 0);
    
    // Status labels
    m_gameStatusLabel = lv_label_create(m_gameContainer);
    lv_label_set_text(m_gameStatusLabel, "Select a game to play");
    lv_obj_align(m_gameStatusLabel, LV_ALIGN_TOP_MID, 0, 20);
    
    m_gameScoreLabel = lv_label_create(m_gameContainer);
    lv_label_set_text(m_gameScoreLabel, "");
    lv_obj_align_to(m_gameScoreLabel, m_gameStatusLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void BasicAppsSuite::createSpreadsheetTab() {
    // Create simple grid for demonstration
    m_spreadsheetGrid = lv_table_create(m_spreadsheetTab);
    lv_obj_set_size(m_spreadsheetGrid, LV_PCT(100), LV_PCT(90));
    lv_obj_align(m_spreadsheetGrid, LV_ALIGN_TOP_MID, 0, 0);
    
    // Setup basic 5x5 grid
    lv_table_set_col_cnt(m_spreadsheetGrid, 5);
    lv_table_set_row_cnt(m_spreadsheetGrid, 5);
    
    // Set headers
    for (int col = 0; col < 5; col++) {
        char header[8];
        sprintf(header, "%c", 'A' + col);
        lv_table_set_cell_value(m_spreadsheetGrid, 0, col, header);
        lv_table_set_col_width(m_spreadsheetGrid, col, 80);
    }
    
    for (int row = 1; row < 5; row++) {
        char header[8];
        sprintf(header, "%d", row);
        lv_table_set_cell_value(m_spreadsheetGrid, row, 0, header);
    }
    
    // Cell reference display
    m_cellReference = lv_label_create(m_spreadsheetTab);
    lv_label_set_text(m_cellReference, "A1");
    lv_obj_align(m_spreadsheetTab, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    
    lv_obj_add_event_cb(m_spreadsheetGrid, cellClickCallback, LV_EVENT_CLICKED, this);
}

void BasicAppsSuite::updateExpenseSummary() {
    lv_obj_clean(m_expenseSummary);
    
    // Calculate totals
    m_totalIncome = 0;
    m_totalExpenses = 0;
    
    for (const auto& expense : m_expenses) {
        if (expense.isIncome) {
            m_totalIncome += expense.amount;
        } else {
            m_totalExpenses += expense.amount;
        }
    }
    
    m_balance = m_totalIncome - m_totalExpenses;
    
    // Create summary labels
    lv_obj_t* incomeLabel = lv_label_create(m_expenseSummary);
    std::string incomeText = "Income: " + formatCurrency(m_totalIncome);
    lv_label_set_text(incomeLabel, incomeText.c_str());
    lv_obj_align(incomeLabel, LV_ALIGN_LEFT_MID, 10, -10);
    lv_obj_set_style_text_color(incomeLabel, lv_color_hex(0x27AE60), 0);
    
    lv_obj_t* expenseLabel = lv_label_create(m_expenseSummary);
    std::string expenseText = "Expenses: " + formatCurrency(m_totalExpenses);
    lv_label_set_text(expenseLabel, expenseText.c_str());
    lv_obj_align(expenseLabel, LV_ALIGN_LEFT_MID, 10, 10);
    lv_obj_set_style_text_color(expenseLabel, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* balanceLabel = lv_label_create(m_expenseSummary);
    std::string balanceText = "Balance: " + formatCurrency(m_balance);
    lv_label_set_text(balanceLabel, balanceText.c_str());
    lv_obj_align(balanceLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(balanceLabel, 
                               m_balance >= 0 ? lv_color_hex(0x27AE60) : lv_color_hex(0xE74C3C), 0);
    lv_obj_set_style_text_font(balanceLabel, &lv_font_montserrat_16, 0);
}

void BasicAppsSuite::createAddExpenseDialog() {
    m_addExpenseDialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_addExpenseDialog, 350, 300);
    lv_obj_center(m_addExpenseDialog);
    lv_obj_set_style_bg_color(m_addExpenseDialog, lv_color_hex(0x2C2C2C), 0);
    
    // Title
    lv_obj_t* title = lv_label_create(m_addExpenseDialog);
    lv_label_set_text(title, m_editingExpense ? "Edit Expense" : "Add Expense");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Description input
    lv_obj_t* descLabel = lv_label_create(m_addExpenseDialog);
    lv_label_set_text(descLabel, "Description:");
    lv_obj_align_to(descLabel, title, LV_ALIGN_OUT_BOTTOM_LEFT, -100, 20);
    
    m_expenseDescInput = lv_textarea_create(m_addExpenseDialog);
    lv_obj_set_size(m_expenseDescInput, 250, 30);
    lv_obj_align_to(m_expenseDescInput, descLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_expenseDescInput, true);
    
    // Amount input
    lv_obj_t* amountLabel = lv_label_create(m_addExpenseDialog);
    lv_label_set_text(amountLabel, "Amount:");
    lv_obj_align_to(amountLabel, m_expenseDescInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_expenseAmountInput = lv_textarea_create(m_addExpenseDialog);
    lv_obj_set_size(m_expenseAmountInput, 120, 30);
    lv_obj_align_to(m_expenseAmountInput, amountLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_expenseAmountInput, true);
    
    // Category dropdown
    lv_obj_t* categoryLabel = lv_label_create(m_addExpenseDialog);
    lv_label_set_text(categoryLabel, "Category:");
    lv_obj_align_to(categoryLabel, amountLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    m_expenseCategoryDropdown = lv_dropdown_create(m_addExpenseDialog);
    lv_obj_set_size(m_expenseCategoryDropdown, 100, 30);
    lv_obj_align_to(m_expenseCategoryDropdown, categoryLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_dropdown_set_options(m_expenseCategoryDropdown, "Food\nTransport\nShopping\nEntertainment\nBills\nOther");
    
    // Income switch
    lv_obj_t* incomeLabel = lv_label_create(m_addExpenseDialog);
    lv_label_set_text(incomeLabel, "Income:");
    lv_obj_align_to(incomeLabel, m_expenseAmountInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    m_expenseIncomeSwitch = lv_switch_create(m_addExpenseDialog);
    lv_obj_align_to(m_expenseIncomeSwitch, incomeLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    // Buttons
    lv_obj_t* saveBtn = lv_btn_create(m_addExpenseDialog);
    lv_obj_set_size(saveBtn, 80, 35);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(saveBtn, saveExpenseCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* saveLabel = lv_label_create(saveBtn);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_center(saveLabel);
    
    lv_obj_t* cancelBtn = lv_btn_create(m_addExpenseDialog);
    lv_obj_set_size(cancelBtn, 80, 35);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(cancelBtn, cancelExpenseCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* cancelLabel = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
    
    // Pre-fill if editing
    if (m_editingExpense) {
        lv_textarea_set_text(m_expenseDescInput, m_editingExpense->description.c_str());
        lv_textarea_set_text(m_expenseAmountInput, std::to_string(m_editingExpense->amount).c_str());
        if (m_editingExpense->isIncome) {
            lv_obj_add_state(m_expenseIncomeSwitch, LV_STATE_CHECKED);
        }
    }
}

void BasicAppsSuite::showAddExpenseDialog(Expense* expense) {
    m_editingExpense = expense;
    createAddExpenseDialog();
}

void BasicAppsSuite::hideAddExpenseDialog() {
    if (m_addExpenseDialog) {
        lv_obj_del(m_addExpenseDialog);
        m_addExpenseDialog = nullptr;
        m_editingExpense = nullptr;
    }
}

void BasicAppsSuite::addExpense(const Expense& expense) {
    Expense newExpense = expense;
    newExpense.id = m_nextExpenseId++;
    newExpense.date = time(nullptr);
    m_expenses.push_back(newExpense);
    
    saveExpenses();
    updateExpenseSummary();
    
    // Refresh expense list
    lv_obj_clean(m_expenseList);
    for (const auto& exp : m_expenses) {
        std::string itemText = exp.description + " - " + formatCurrency(exp.amount);
        if (exp.isIncome) {
            itemText = "+ " + itemText;
        } else {
            itemText = "- " + itemText;
        }
        
        lv_obj_t* btn = lv_list_add_btn(m_expenseList, nullptr, itemText.c_str());
        lv_obj_set_user_data(btn, (void*)(uintptr_t)exp.id);
        lv_obj_add_event_cb(btn, expenseListCallback, LV_EVENT_CLICKED, this);
        
        if (exp.isIncome) {
            lv_obj_set_style_text_color(btn, lv_color_hex(0x27AE60), 0);
        } else {
            lv_obj_set_style_text_color(btn, lv_color_hex(0xE74C3C), 0);
        }
    }
}

std::string BasicAppsSuite::formatCurrency(float amount) {
    std::stringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << amount;
    return ss.str();
}

std::string BasicAppsSuite::formatDate(time_t date) {
    struct tm* timeinfo = localtime(&date);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%m/%d/%Y", timeinfo);
    return std::string(buffer);
}

os_error_t BasicAppsSuite::loadExpenses() {
    // Add sample expenses
    Expense expense1 = {m_nextExpenseId++, "Grocery shopping", 45.67f, "Food", time(nullptr) - 86400, false};
    Expense expense2 = {m_nextExpenseId++, "Salary", 2500.0f, "Income", time(nullptr) - 2*86400, true};
    Expense expense3 = {m_nextExpenseId++, "Gas", 35.20f, "Transport", time(nullptr) - 3*86400, false};
    
    m_expenses.push_back(expense1);
    m_expenses.push_back(expense2);
    m_expenses.push_back(expense3);
    
    return OS_OK;
}

os_error_t BasicAppsSuite::saveExpenses() {
    // In a real implementation, save to file system
    return OS_OK;
}

os_error_t BasicAppsSuite::loadSpreadsheet() {
    // In a real implementation, load from file system
    return OS_OK;
}

os_error_t BasicAppsSuite::saveSpreadsheet() {
    // In a real implementation, save to file system
    return OS_OK;
}

void BasicAppsSuite::calculateResult() {
    try {
        float result = evaluateExpression(m_calculatorExpression);
        m_calculatorResult = std::to_string(result);
        
        // Format result to remove unnecessary decimal places
        if (result == floor(result)) {
            m_calculatorResult = std::to_string((int)result);
        } else {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(6) << result;
            m_calculatorResult = ss.str();
            
            // Remove trailing zeros
            m_calculatorResult.erase(m_calculatorResult.find_last_not_of('0') + 1, std::string::npos);
            m_calculatorResult.erase(m_calculatorResult.find_last_not_of('.') + 1, std::string::npos);
        }
        
        lv_textarea_set_text(m_calculatorDisplay, m_calculatorResult.c_str());
        m_calculatorNewInput = true;
    } catch (...) {
        lv_textarea_set_text(m_calculatorDisplay, "Error");
        m_calculatorNewInput = true;
    }
}

float BasicAppsSuite::evaluateExpression(const std::string& expression) {
    // Simple expression evaluator - for demo purposes
    // In a real implementation, would use a proper parser
    std::string expr = expression;
    
    // Replace symbols with operators - use string replacement for Unicode
    size_t pos = expr.find("×");
    while (pos != std::string::npos) {
        expr.replace(pos, 3, "*"); // × is 3 bytes in UTF-8
        pos = expr.find("×", pos + 1);
    }
    
    pos = expr.find("÷");
    while (pos != std::string::npos) {
        expr.replace(pos, 3, "/"); // ÷ is 3 bytes in UTF-8
        pos = expr.find("÷", pos + 1);
    }
    
    // Very basic evaluation - only handles simple operations
    size_t opPos = std::string::npos;
    char op = 0;
    
    // Find last operator (right to left for correct precedence in simple cases)
    for (int i = expr.length() - 1; i >= 0; i--) {
        if (expr[i] == '+' || expr[i] == '-') {
            opPos = i;
            op = expr[i];
            break;
        }
    }
    
    if (opPos == std::string::npos) {
        for (int i = expr.length() - 1; i >= 0; i--) {
            if (expr[i] == '*' || expr[i] == '/') {
                opPos = i;
                op = expr[i];
                break;
            }
        }
    }
    
    if (opPos != std::string::npos && opPos > 0) {
        float left = evaluateExpression(expr.substr(0, opPos));
        float right = evaluateExpression(expr.substr(opPos + 1));
        
        switch (op) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return right != 0 ? left / right : 0;
        }
    }
    
    return std::stof(expr);
}

// Static callbacks
void BasicAppsSuite::tabChangedCallback(lv_event_t* e) {
    // Handle tab change if needed
}

void BasicAppsSuite::addExpenseCallback(lv_event_t* e) {
    BasicAppsSuite* app = static_cast<BasicAppsSuite*>(lv_event_get_user_data(e));
    app->showAddExpenseDialog();
}

void BasicAppsSuite::saveExpenseCallback(lv_event_t* e) {
    BasicAppsSuite* app = static_cast<BasicAppsSuite*>(lv_event_get_user_data(e));
    
    Expense expense;
    expense.description = lv_textarea_get_text(app->m_expenseDescInput);
    expense.amount = app->parseFloat(lv_textarea_get_text(app->m_expenseAmountInput));
    expense.isIncome = lv_obj_has_state(app->m_expenseIncomeSwitch, LV_STATE_CHECKED);
    
    // Get category
    uint16_t selected = lv_dropdown_get_selected(app->m_expenseCategoryDropdown);
    const char* categories[] = {"Food", "Transport", "Shopping", "Entertainment", "Bills", "Other"};
    expense.category = categories[selected];
    
    if (app->m_editingExpense) {
        app->editExpense(app->m_editingExpense->id, expense);
    } else {
        app->addExpense(expense);
    }
    
    app->hideAddExpenseDialog();
}

void BasicAppsSuite::cancelExpenseCallback(lv_event_t* e) {
    BasicAppsSuite* app = static_cast<BasicAppsSuite*>(lv_event_get_user_data(e));
    app->hideAddExpenseDialog();
}

void BasicAppsSuite::expenseListCallback(lv_event_t* e) {
    // Handle expense list item click
}

void BasicAppsSuite::categoryFilterCallback(lv_event_t* e) {
    // Handle category filter change
}

void BasicAppsSuite::cellClickCallback(lv_event_t* e) {
    // Handle spreadsheet cell click
}

void BasicAppsSuite::calculatorButtonCallback(lv_event_t* e) {
    BasicAppsSuite* app = static_cast<BasicAppsSuite*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    const char* btnText = (const char*)lv_obj_get_user_data(btn);
    
    if (strcmp(btnText, "C") == 0) {
        app->m_calculatorExpression = "";
        lv_textarea_set_text(app->m_calculatorDisplay, "0");
        app->m_calculatorNewInput = true;
    } else if (strcmp(btnText, "=") == 0) {
        if (!app->m_calculatorExpression.empty()) {
            app->calculateResult();
        }
    } else if (strcmp(btnText, "+") == 0 || strcmp(btnText, "-") == 0 || 
               strcmp(btnText, "×") == 0 || strcmp(btnText, "÷") == 0) {
        if (app->m_calculatorNewInput && !app->m_calculatorResult.empty()) {
            app->m_calculatorExpression = app->m_calculatorResult + btnText;
        } else {
            app->m_calculatorExpression += btnText;
        }
        lv_textarea_set_text(app->m_calculatorDisplay, app->m_calculatorExpression.c_str());
        app->m_calculatorNewInput = false;
    } else {
        // Number or decimal point
        if (app->m_calculatorNewInput) {
            app->m_calculatorExpression = btnText;
            app->m_calculatorNewInput = false;
        } else {
            app->m_calculatorExpression += btnText;
        }
        lv_textarea_set_text(app->m_calculatorDisplay, app->m_calculatorExpression.c_str());
    }
}

float BasicAppsSuite::parseFloat(const std::string& str) {
    try {
        return std::stof(str);
    } catch (...) {
        return 0.0f;
    }
}

void BasicAppsSuite::editExpense(uint32_t expenseId, const Expense& expense) {
    for (auto& exp : m_expenses) {
        if (exp.id == expenseId) {
            exp.description = expense.description;
            exp.amount = expense.amount;
            exp.category = expense.category;
            exp.isIncome = expense.isIncome;
            break;
        }
    }
    saveExpenses();
    updateExpenseSummary();
}

extern "C" std::unique_ptr<BaseApp> createBasicAppsSuite() {
    return std::make_unique<BasicAppsSuite>();
}