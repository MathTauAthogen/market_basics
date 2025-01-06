#pragma once

#include <unordered_map>
#include <map>
#include <functional>
#include <vector>
#include <string>


using Symbol = int;

enum class Side {
    BUY, SELL
};


// QuickAccessMap helper class
// Updates, Lookups, and gets the greatest k elements in O(1) time or O(k) time.
// Inserts and erases in O(n) time.

template <typename T, typename E>
class QuickAccessMap {
public:
    QuickAccessMap () = default;
    QuickAccessMap (std::function<bool(const T&, const T&)> comp) : baseMap {comp} {}

    auto begin () {return baseMap.begin();}
    auto end () {return baseMap.end();}
    bool contains (T t) {return quickAccess.count(t);}

    bool insert (T t, E e = E{});
    bool erase (T t);

    typename std::map<T, E>::iterator get_it (T t);

    E& operator[] (T t) {return get_it(t)->second;}
    E get (T t) {return operator[] (t);}

private:
    std::unordered_map <T, typename std::map<T, E>::iterator> quickAccess;
    std::map <T, E, std::function<bool(const T&, const T&)>> baseMap;
};

class Book {
public:
    Book (std::function<bool(const int&, const int &)> comp) : levels {comp} {}
    Book () = default;

    struct Order;

    struct BookLevel {
        BookLevel () = default; // Don't use but needed for map?
        BookLevel (Symbol _symbol, int _px, Side _side) : symbol{_symbol}, px{_px}, qty{0}, num{0}, side{_side} {}

        int px;
        int qty;
        int num;
        Symbol symbol;
        Side side;
        std::vector <Order*> orders;
    };

    struct Order {
        Order () = default; // Don't use but needed for map?
        Order (int _id, int _px, int _qty, Side _side, Symbol _symbol) : side{_side}, id{_id}, px{_px}, qty{_qty}, symbol{_symbol} {}

        int id;
        int px;
        int qty;
        Side side;
        int in_level;
        Symbol symbol;
        std::map <int, BookLevel>::iterator it;      
    };


    QuickAccessMap <int, BookLevel>& get_levels () {return levels;}
private:
    QuickAccessMap <int, BookLevel> levels;
};


class SymbolHandler {
public:
    SymbolHandler () = default; // Don't use but needed for map?
    SymbolHandler (Symbol symbol);

    std::pair<std::vector<Book::BookLevel>, int> queryBookFullInfo(int depth, Side side);

    // Returns: the number of levels successfully provided (depth if there are at least depth non-zero levels, otherwise less)
    int queryBookFullInfo(int depth, Side side, std::vector<Book::BookLevel>& bookLevels); // Please supply your own vector of requisite size; I perform no bounds-checking.
    // bookLevels is just assumed to have the memory for me already allocated - the caller is responsible for that.

    std::pair<std::vector<std::pair<int, int>>, int> queryBook(int depth, Side side);

    // Returns: the number of levels successfully provided (depth if there are at least depth non-zero levels, otherwise less)
    int queryBook(int depth, Side side, std::vector<std::pair<int, int>>& bookLevels); // Please supply your own vector of requisite size; I perform no bounds-checking.
    // bookLevels is just assumed to have the memory for me already allocated - the caller is responsible for that.

    bool addOrder (int orderId, int px, int qty, Side side);

    bool addOrder (const Book::Order& order);

    bool removeOrder (int orderId);

    bool changeOrder (int orderId, int px, int qty);
    
private:

    void handleAddedOrder (Book::Order& order, int px, Side side);

    Symbol m_symbol;
    Book books [2]; // Buy book is element 0 and Sell book is element 1
};


class BookManager {
public:  
    BookManager () = delete;

    static Symbol getSymbolId (const std::string& symbol) {return symbolLookup[symbol];}
    static const std::string& getSymbolName (Symbol symbol) {return symbolBack[symbol];}
    static SymbolHandler& getSymbolHandler (Symbol symbol) {return symbolBooks[symbol];}
    static bool is_orderId (int orderId) {return orders.find(orderId) != orders.end();}
    static size_t getNumberofSymbols () {return symbols;}

    static bool addSymbol (Symbol symbolId, const std::string& symbol);

    static std::unordered_map<int, Book::Order>::iterator add_order (int orderId, Book::Order order);
    static std::unordered_map<int, Book::Order>::iterator emplace_order (int orderId, int px, int qty, Side side, Symbol symbol);

    static Book::Order& get_order (int orderId);

    static bool delete_order (int orderId);    

private:
    static int symbols;

    static std::unordered_map<int, Book::Order>::iterator nullIt;
    static Book::Order nullOrder;

    static std::unordered_map <Symbol, SymbolHandler> symbolBooks;
    static std::unordered_map <std::string, Symbol> symbolLookup;
    static std::unordered_map <Symbol, std::string> symbolBack;
    static std::unordered_map <int, Book::Order> orders;
};


template <typename T, typename E>
bool QuickAccessMap<T, E>::insert (T t, E e) {
    if (quickAccess.find(t) != quickAccess.end()) {
        return false;
    }
    quickAccess[t] = baseMap.insert({t, e}).first;
    return true;
}

template <typename T, typename E>
bool QuickAccessMap<T, E>::erase (T t) {
    if (quickAccess.find(t) == quickAccess.end()) {
        return false;
    }
    baseMap.erase(t);
    quickAccess.erase(t);
}

template <typename T, typename E>
typename std::map<T, E>::iterator QuickAccessMap<T, E>::get_it (T t) {
    if (!contains(t)) {
        throw std::domain_error ("Key " + std::to_string(t) + " not found");
    }
    return quickAccess[t];
}