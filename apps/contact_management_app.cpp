#include "contact_management_app.h"
#include "../system/os_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ContactManagementApp::ContactManagementApp() 
    : BaseApp("com.m5stack.contacts", "Contacts", "1.0.0"), m_nextContactId(1) {
    setDescription("Contact management application with address book functionality");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

ContactManagementApp::~ContactManagementApp() {
    shutdown();
}

os_error_t ContactManagementApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Contact Management App");
    
    // Load existing contacts
    loadContacts();
    
    // Set memory usage estimate
    setMemoryUsage(32768); // 32KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t ContactManagementApp::update(uint32_t deltaTime) {
    // Periodic operations can be added here
    return OS_OK;
}

os_error_t ContactManagementApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Contact Management App");
    
    // Save contacts before shutdown
    saveContacts();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t ContactManagementApp::createUI(lv_obj_t* parent) {
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
    lv_obj_set_style_pad_all(m_uiContainer, 10, 0);

    createMainUI();
    return OS_OK;
}

os_error_t ContactManagementApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void ContactManagementApp::createMainUI() {
    createSearchBar();
    createToolbar();
    createContactList();
    createContactDetails();
    
    // Initially hide details panel
    lv_obj_add_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
    
    refreshContactList();
}

void ContactManagementApp::createSearchBar() {
    // Search container
    lv_obj_t* searchContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(searchContainer, LV_PCT(100), 50);
    lv_obj_align(searchContainer, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(searchContainer, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(searchContainer, 5, 0);
    
    // Search input
    m_searchBar = lv_textarea_create(searchContainer);
    lv_obj_set_size(m_searchBar, LV_PCT(70), LV_PCT(100));
    lv_obj_align(m_searchBar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_textarea_set_placeholder_text(m_searchBar, "Search contacts...");
    lv_textarea_set_one_line(m_searchBar, true);
    lv_obj_add_event_cb(m_searchBar, searchCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Category filter dropdown
    m_categoryDropdown = lv_dropdown_create(searchContainer);
    lv_obj_set_size(m_categoryDropdown, LV_PCT(25), LV_PCT(100));
    lv_obj_align(m_categoryDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_dropdown_set_options(m_categoryDropdown, "All\nFamily\nFriends\nWork\nOther");
    lv_obj_add_event_cb(m_categoryDropdown, categoryFilterCallback, LV_EVENT_VALUE_CHANGED, this);
}

void ContactManagementApp::createToolbar() {
    m_toolbar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_toolbar, LV_PCT(100), 50);
    lv_obj_align_to(m_toolbar, m_searchBar->parent, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_toolbar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(m_toolbar, 5, 0);
    
    // Add button
    lv_obj_t* addBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(addBtn, 80, LV_PCT(100));
    lv_obj_align(addBtn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(addBtn, addButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* addLabel = lv_label_create(addBtn);
    lv_label_set_text(addLabel, "Add");
    lv_obj_center(addLabel);
    
    // Edit button
    lv_obj_t* editBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(editBtn, 80, LV_PCT(100));
    lv_obj_align_to(editBtn, addBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(editBtn, editButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* editLabel = lv_label_create(editBtn);
    lv_label_set_text(editLabel, "Edit");
    lv_obj_center(editLabel);
    
    // Delete button
    lv_obj_t* deleteBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(deleteBtn, 80, LV_PCT(100));
    lv_obj_align_to(deleteBtn, editBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(deleteBtn, deleteButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(deleteBtn, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* deleteLabel = lv_label_create(deleteBtn);
    lv_label_set_text(deleteLabel, "Delete");
    lv_obj_center(deleteLabel);
}

void ContactManagementApp::createContactList() {
    m_contactList = lv_list_create(m_uiContainer);
    lv_obj_set_size(m_contactList, LV_PCT(50), LV_PCT(100) - 110);
    lv_obj_align_to(m_contactList, m_toolbar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_contactList, lv_color_hex(0x2C2C2C), 0);
}

void ContactManagementApp::createContactDetails() {
    m_detailsPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_detailsPanel, LV_PCT(45), LV_PCT(100) - 110);
    lv_obj_align_to(m_detailsPanel, m_contactList, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(m_detailsPanel, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(m_detailsPanel, 15, 0);
}

void ContactManagementApp::refreshContactList() {
    lv_obj_clean(m_contactList);
    
    const std::vector<Contact>& contacts = m_currentFilter.empty() ? m_contacts : m_filteredContacts;
    
    for (const auto& contact : contacts) {
        lv_obj_t* btn = lv_list_add_btn(m_contactList, LV_SYMBOL_CALL, contact.name.c_str());
        lv_obj_set_user_data(btn, (void*)contact.id);
        lv_obj_add_event_cb(btn, contactListCallback, LV_EVENT_CLICKED, this);
        
        // Add category indicator
        if (!contact.category.empty()) {
            lv_obj_t* categoryLabel = lv_label_create(btn);
            lv_label_set_text(categoryLabel, contact.category.c_str());
            lv_obj_align(categoryLabel, LV_ALIGN_BOTTOM_RIGHT, -5, -2);
            lv_obj_set_style_text_color(categoryLabel, lv_color_hex(0x95A5A6), 0);
            lv_obj_set_style_text_font(categoryLabel, &lv_font_montserrat_12, 0);
        }
    }
}

void ContactManagementApp::showContactDetails(const Contact& contact) {
    lv_obj_clean(m_detailsPanel);
    lv_obj_clear_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
    
    // Title
    lv_obj_t* titleLabel = lv_label_create(m_detailsPanel);
    lv_label_set_text(titleLabel, contact.name.c_str());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, 0);
    
    // Phone
    if (!contact.phone.empty()) {
        lv_obj_t* phoneLabel = lv_label_create(m_detailsPanel);
        std::string phoneText = LV_SYMBOL_CALL " " + contact.phone;
        lv_label_set_text(phoneLabel, phoneText.c_str());
        lv_obj_align_to(phoneLabel, titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    }
    
    // Email
    if (!contact.email.empty()) {
        lv_obj_t* emailLabel = lv_label_create(m_detailsPanel);
        std::string emailText = LV_SYMBOL_ENVELOPE " " + contact.email;
        lv_label_set_text(emailLabel, emailText.c_str());
        lv_obj_align_to(emailLabel, titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 50);
    }
    
    // Address
    if (!contact.address.empty()) {
        lv_obj_t* addressLabel = lv_label_create(m_detailsPanel);
        std::string addressText = LV_SYMBOL_HOME " " + contact.address;
        lv_label_set_text(addressLabel, addressText.c_str());
        lv_obj_align_to(addressLabel, titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 80);
        lv_label_set_long_mode(addressLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(addressLabel, LV_PCT(90));
    }
    
    // Category
    if (!contact.category.empty()) {
        lv_obj_t* categoryLabel = lv_label_create(m_detailsPanel);
        std::string categoryText = LV_SYMBOL_LIST " " + contact.category;
        lv_label_set_text(categoryLabel, categoryText.c_str());
        lv_obj_align(categoryLabel, LV_ALIGN_BOTTOM_LEFT, 0, -10);
    }
}

void ContactManagementApp::createAddEditDialog() {
    m_addEditDialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_addEditDialog, 400, 350);
    lv_obj_center(m_addEditDialog);
    lv_obj_set_style_bg_color(m_addEditDialog, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_color(m_addEditDialog, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_border_width(m_addEditDialog, 2, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(m_addEditDialog);
    lv_label_set_text(title, m_editingContact ? "Edit Contact" : "Add Contact");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Name input
    lv_obj_t* nameLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(nameLabel, "Name:");
    lv_obj_align_to(nameLabel, title, LV_ALIGN_OUT_BOTTOM_LEFT, -150, 20);
    
    m_nameInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_nameInput, 300, 30);
    lv_obj_align_to(m_nameInput, nameLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_nameInput, true);
    
    // Phone input
    lv_obj_t* phoneLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(phoneLabel, "Phone:");
    lv_obj_align_to(phoneLabel, m_nameInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_phoneInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_phoneInput, 300, 30);
    lv_obj_align_to(m_phoneInput, phoneLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_phoneInput, true);
    
    // Email input
    lv_obj_t* emailLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(emailLabel, "Email:");
    lv_obj_align_to(emailLabel, m_phoneInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_emailInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_emailInput, 300, 30);
    lv_obj_align_to(m_emailInput, emailLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_emailInput, true);
    
    // Address input
    lv_obj_t* addressLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(addressLabel, "Address:");
    lv_obj_align_to(addressLabel, m_emailInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_addressInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_addressInput, 300, 40);
    lv_obj_align_to(m_addressInput, addressLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    // Category input
    lv_obj_t* categoryLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(categoryLabel, "Category:");
    lv_obj_align_to(categoryLabel, m_addressInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_categoryInput = lv_dropdown_create(m_addEditDialog);
    lv_obj_set_size(m_categoryInput, 150, 30);
    lv_obj_align_to(m_categoryInput, categoryLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_dropdown_set_options(m_categoryInput, "Family\nFriends\nWork\nOther");
    
    // Buttons
    lv_obj_t* saveBtn = lv_btn_create(m_addEditDialog);
    lv_obj_set_size(saveBtn, 80, 35);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(saveBtn, saveButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(saveBtn, lv_color_hex(0x27AE60), 0);
    
    lv_obj_t* saveLabel = lv_label_create(saveBtn);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_center(saveLabel);
    
    lv_obj_t* cancelBtn = lv_btn_create(m_addEditDialog);
    lv_obj_set_size(cancelBtn, 80, 35);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(cancelBtn, cancelButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(cancelBtn, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* cancelLabel = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
    
    // Pre-fill if editing
    if (m_editingContact) {
        lv_textarea_set_text(m_nameInput, m_editingContact->name.c_str());
        lv_textarea_set_text(m_phoneInput, m_editingContact->phone.c_str());
        lv_textarea_set_text(m_emailInput, m_editingContact->email.c_str());
        lv_textarea_set_text(m_addressInput, m_editingContact->address.c_str());
        
        // Set category
        if (m_editingContact->category == "Family") lv_dropdown_set_selected(m_categoryInput, 0);
        else if (m_editingContact->category == "Friends") lv_dropdown_set_selected(m_categoryInput, 1);
        else if (m_editingContact->category == "Work") lv_dropdown_set_selected(m_categoryInput, 2);
        else lv_dropdown_set_selected(m_categoryInput, 3);
    }
}

void ContactManagementApp::showAddEditDialog(Contact* contact) {
    m_editingContact = contact;
    createAddEditDialog();
}

void ContactManagementApp::hideAddEditDialog() {
    if (m_addEditDialog) {
        lv_obj_del(m_addEditDialog);
        m_addEditDialog = nullptr;
        m_editingContact = nullptr;
    }
}

void ContactManagementApp::searchContacts(const std::string& searchText) {
    m_currentFilter = searchText;
    m_filteredContacts.clear();
    
    if (searchText.empty()) {
        refreshContactList();
        return;
    }
    
    std::string lowerSearch = searchText;
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
    
    for (const auto& contact : m_contacts) {
        std::string lowerName = contact.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(lowerSearch) != std::string::npos ||
            contact.phone.find(searchText) != std::string::npos ||
            contact.email.find(lowerSearch) != std::string::npos) {
            
            if (m_currentCategory.empty() || contact.category == m_currentCategory) {
                m_filteredContacts.push_back(contact);
            }
        }
    }
    
    refreshContactList();
}

void ContactManagementApp::filterByCategory(const std::string& category) {
    m_currentCategory = (category == "All") ? "" : category;
    
    if (!m_currentFilter.empty()) {
        searchContacts(m_currentFilter);
    } else {
        m_filteredContacts.clear();
        
        if (m_currentCategory.empty()) {
            refreshContactList();
            return;
        }
        
        for (const auto& contact : m_contacts) {
            if (contact.category == m_currentCategory) {
                m_filteredContacts.push_back(contact);
            }
        }
        
        refreshContactList();
    }
}

void ContactManagementApp::addContact(const Contact& contact) {
    Contact newContact = contact;
    newContact.id = m_nextContactId++;
    m_contacts.push_back(newContact);
    saveContacts();
    refreshContactList();
}

void ContactManagementApp::editContact(uint32_t contactId, const Contact& contact) {
    for (auto& c : m_contacts) {
        if (c.id == contactId) {
            c.name = contact.name;
            c.phone = contact.phone;
            c.email = contact.email;
            c.address = contact.address;
            c.category = contact.category;
            break;
        }
    }
    saveContacts();
    refreshContactList();
}

void ContactManagementApp::deleteContact(uint32_t contactId) {
    m_contacts.erase(
        std::remove_if(m_contacts.begin(), m_contacts.end(),
                      [contactId](const Contact& c) { return c.id == contactId; }),
        m_contacts.end());
    saveContacts();
    refreshContactList();
    lv_obj_add_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
}

os_error_t ContactManagementApp::loadContacts() {
    // For now, add some sample contacts
    Contact contact1 = {"John Doe", "+1234567890", "john@example.com", "123 Main St", "Work", m_nextContactId++};
    Contact contact2 = {"Jane Smith", "+0987654321", "jane@example.com", "456 Oak Ave", "Friends", m_nextContactId++};
    Contact contact3 = {"Bob Johnson", "+1122334455", "bob@company.com", "789 Pine Rd", "Work", m_nextContactId++};
    
    m_contacts.push_back(contact1);
    m_contacts.push_back(contact2);
    m_contacts.push_back(contact3);
    
    return OS_OK;
}

os_error_t ContactManagementApp::saveContacts() {
    // In a real implementation, save to file system
    return OS_OK;
}

// Static callbacks
void ContactManagementApp::searchCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    lv_obj_t* textarea = lv_event_get_target(e);
    const char* text = lv_textarea_get_text(textarea);
    app->searchContacts(text ? text : "");
}

void ContactManagementApp::contactListCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    uint32_t contactId = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);
    
    for (const auto& contact : app->m_contacts) {
        if (contact.id == contactId) {
            app->showContactDetails(contact);
            break;
        }
    }
}

void ContactManagementApp::addButtonCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    app->showAddEditDialog();
}

void ContactManagementApp::editButtonCallback(lv_event_t* e) {
    // Implementation for edit button - would need to get currently selected contact
}

void ContactManagementApp::deleteButtonCallback(lv_event_t* e) {
    // Implementation for delete button - would need to get currently selected contact
}

void ContactManagementApp::saveButtonCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    
    Contact contact;
    contact.name = lv_textarea_get_text(app->m_nameInput);
    contact.phone = lv_textarea_get_text(app->m_phoneInput);
    contact.email = lv_textarea_get_text(app->m_emailInput);
    contact.address = lv_textarea_get_text(app->m_addressInput);
    
    // Get category
    uint16_t selected = lv_dropdown_get_selected(app->m_categoryInput);
    switch (selected) {
        case 0: contact.category = "Family"; break;
        case 1: contact.category = "Friends"; break;
        case 2: contact.category = "Work"; break;
        default: contact.category = "Other"; break;
    }
    
    if (app->m_editingContact) {
        app->editContact(app->m_editingContact->id, contact);
    } else {
        app->addContact(contact);
    }
    
    app->hideAddEditDialog();
}

void ContactManagementApp::cancelButtonCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    app->hideAddEditDialog();
}

void ContactManagementApp::categoryFilterCallback(lv_event_t* e) {
    ContactManagementApp* app = static_cast<ContactManagementApp*>(lv_event_get_user_data(e));
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    
    const char* categories[] = {"All", "Family", "Friends", "Work", "Other"};
    app->filterByCategory(categories[selected]);
}

extern "C" std::unique_ptr<BaseApp> createContactManagementApp() {
    return std::make_unique<ContactManagementApp>();
}