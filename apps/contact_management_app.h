#ifndef CONTACT_MANAGEMENT_APP_H
#define CONTACT_MANAGEMENT_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <functional>

struct Contact {
    std::string name;
    std::string phone;
    std::string email;
    std::string address;
    std::string category;
    uint32_t id;
};

class ContactManagementApp : public BaseApp {
public:
    ContactManagementApp();
    ~ContactManagementApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void createContactList();
    void createSearchBar();
    void createToolbar();
    void createContactDetails();
    void createAddEditDialog();
    
    void refreshContactList();
    void showContactDetails(const Contact& contact);
    void showAddEditDialog(Contact* contact = nullptr);
    void hideAddEditDialog();
    void searchContacts(const std::string& searchText);
    void filterByCategory(const std::string& category);
    
    void addContact(const Contact& contact);
    void editContact(uint32_t contactId, const Contact& contact);
    void deleteContact(uint32_t contactId);
    
    os_error_t loadContacts();
    os_error_t saveContacts();
    
    static void searchCallback(lv_event_t* e);
    static void contactListCallback(lv_event_t* e);
    static void addButtonCallback(lv_event_t* e);
    static void editButtonCallback(lv_event_t* e);
    static void deleteButtonCallback(lv_event_t* e);
    static void saveButtonCallback(lv_event_t* e);
    static void cancelButtonCallback(lv_event_t* e);
    static void categoryFilterCallback(lv_event_t* e);
    
    std::vector<Contact> m_contacts;
    std::vector<Contact> m_filteredContacts;
    uint32_t m_nextContactId;
    
    // UI elements
    lv_obj_t* m_searchBar = nullptr;
    lv_obj_t* m_contactList = nullptr;
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_detailsPanel = nullptr;
    lv_obj_t* m_addEditDialog = nullptr;
    lv_obj_t* m_categoryDropdown = nullptr;
    
    // Add/Edit dialog elements
    lv_obj_t* m_nameInput = nullptr;
    lv_obj_t* m_phoneInput = nullptr;
    lv_obj_t* m_emailInput = nullptr;
    lv_obj_t* m_addressInput = nullptr;
    lv_obj_t* m_categoryInput = nullptr;
    
    Contact* m_editingContact = nullptr;
    std::string m_currentFilter;
    std::string m_currentCategory;
};

#endif // CONTACT_MANAGEMENT_APP_H