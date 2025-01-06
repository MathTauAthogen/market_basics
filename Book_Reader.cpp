#include "Book_Reader.h"
#include <iostream>

using namespace std;

int BookManager::symbols;

unordered_map<int, Book::Order>::iterator BookManager::nullIt;
Book::Order nullOrder;

unordered_map <Symbol, SymbolHandler> BookManager::symbolBooks;
unordered_map <std::string, Symbol> BookManager::symbolLookup;
unordered_map <Symbol, std::string> BookManager::symbolBack;
unordered_map <int, Book::Order> BookManager::orders;

SymbolHandler::SymbolHandler (Symbol symbol) : m_symbol {symbol}
{
    books[0] = Book(
            bind(
                mem_fn(&greater<int>::operator()),
                greater<int>(),
                placeholders::_1,
                placeholders::_2
            )
        );

    books[1] = Book();
}

int SymbolHandler::queryBookFullInfo(int depth, Side side, vector<Book::BookLevel> & bookLevels) {
    auto levels = books[(int)side].get_levels();
    auto beg = levels.begin();
    auto end = levels.end();
    int i;
    Book::BookLevel curr;
    for (i = 0; (i < depth) && (beg != end); i++) {
        curr = (beg++)->second;
        while ((curr.num == 0) && (beg != end)) {
            curr = beg->second;
            beg ++;
        }
        if (curr.num != 0) {
            bookLevels[i] = curr;
        }
        if (beg == end) {
            i ++;
            break;
        }
    }

    return i; // One beyond the last index added, so actually the number of entries actually loaded.
}

pair<vector<Book::BookLevel>, int> SymbolHandler::queryBookFullInfo(int depth, Side side) {
    vector<Book::BookLevel> bookLevels (depth);
    int i = queryBookFullInfo (depth, side, bookLevels);
    return {bookLevels, i};
}


int SymbolHandler::queryBook(int depth, Side side, vector<pair<int, int>>& bookLevels) {
    auto beg = books[(int)side].get_levels().begin();
    auto end = books[(int)side].get_levels().end();
    int i;
    auto curr = beg->second;
    for (i = 0; (i < depth) && (beg != end); i++) {
        curr = (beg++)->second;
        while ((curr.num == 0) && (beg != end)) {
            curr = beg->second;
            beg ++;
        }
        if (curr.num != 0) {
            bookLevels[i] = {curr.px, curr.qty};
        }
        if (beg == end) {
            i ++;
            break;
        }
    }

    return i; // One beyond the last index added, so actually the number of entries actually loaded.
}

pair<vector<pair<int, int>>, int> SymbolHandler::queryBook(int depth, Side side) {
    vector<pair<int, int>> bookLevels (depth);
    int i = queryBook (depth, side, bookLevels);
    return {bookLevels, i};
}

void SymbolHandler::handleAddedOrder (Book::Order& order, int px, Side side) {
    auto& levs = books[(int) side].get_levels();

    if (!levs.contains(px)) {
        levs.insert(px, Book::BookLevel{m_symbol, px, side});
    }

    auto& bookLevel = levs[px];    

    order.in_level = bookLevel.orders.size();
    order.it = levs.get_it(px);

    bookLevel.num += 1;
    bookLevel.qty += order.qty;
    bookLevel.orders.push_back(&order);
}

bool SymbolHandler::addOrder (int orderId, int px, int qty, Side side) {
    if (BookManager::is_orderId(orderId)) {
        return false;
    }

    auto& order = BookManager::emplace_order(orderId, px, qty, side, m_symbol)->second;

    handleAddedOrder (order, px, side);

    return true;
}

bool SymbolHandler::addOrder (const Book::Order& addingorder) {
    int orderId = addingorder.id;

    if (BookManager::is_orderId(orderId) || addingorder.symbol != m_symbol) {
        return false;
    }

    Side side = addingorder.side;
    int px = addingorder.px;
    
    auto& order = BookManager::add_order(orderId, addingorder)->second;

     handleAddedOrder (order, px, side);

    return true;
}

bool SymbolHandler::removeOrder (int orderId) {
    if (!BookManager::is_orderId(orderId)) {
        return false;
    }

    auto& order = BookManager::get_order(orderId);
    if (order.symbol != m_symbol) {
        return false;
    }

    auto& bookLevel = books[(int) order.side].get_levels()[order.px];
    bookLevel.qty -= order.qty;
    bookLevel.num -= 1;
    bookLevel.orders[order.in_level] = nullptr; // Invalidate order in level
    BookManager::delete_order(orderId);
    return true;
}

bool SymbolHandler::changeOrder (int orderId, int px, int qty) {
    if (!BookManager::is_orderId(orderId)) {
        return false;
    }

    auto& order = BookManager::get_order(orderId);
    if (order.symbol != m_symbol) {
        return false;
    }

    if (px == order.px) {
        auto& bookLevel = books[(int) order.side].get_levels()[px];
        bookLevel.qty += (qty - order.qty);
        order.qty = qty;
        bookLevel.orders[order.in_level]->qty = qty;
        return true;
    }

    removeOrder(orderId);
    addOrder(orderId, px, qty, order.side);
    return true;
}

bool BookManager::addSymbol (Symbol symbolId, const string& symbol) {
    if (symbolBack.count(symbolId)) {
        return false;
    }
    symbols += 1;
    symbolBack[symbolId] = symbol;
    symbolLookup[symbol] = symbolId;
    symbolBooks[symbolId] = SymbolHandler(symbolId);
    return true;
}

unordered_map<int, Book::Order>::iterator BookManager::add_order (int orderId, Book::Order order) {
    if (is_orderId(orderId)){
        return nullIt;
    }
    return orders.insert({orderId, order}).first;
}

unordered_map<int, Book::Order>::iterator BookManager::emplace_order (int orderId, int px, int qty, Side side, Symbol symbol) {
    if (is_orderId(orderId)){
        return nullIt;
    }
    return orders.emplace(piecewise_construct, forward_as_tuple(orderId), forward_as_tuple(orderId, px, qty, side, symbol)).first;
}

Book::Order& BookManager::get_order (int orderId) {
    if (!is_orderId) {
        return nullOrder;
    }
    return orders[orderId];
}

bool BookManager::delete_order (int orderId) {
    if (!is_orderId(orderId)) {
        return false;
    }
    orders.erase(orderId);
    return true;
}


void printBookLevel (Book::BookLevel& level) {
    cout << "Book level for " << BookManager::getSymbolName(level.symbol) << ":" <<  endl;
    cout << "This level has price " << level.px << " and is on the " << (level.side == Side::BUY ? "buy" : "sell") << " side." << endl;
    cout << "There is/are " << level.num << " order(s) on this level, totalling a quantity of " << level.qty << ". It is/They are: " << endl;
    for (auto& i : level.orders) {
        if (i != nullptr) {
            cout << "   Order with id " << i->id << ": quantity = " << i->qty << endl;
        }
    }
}


int main () {
    BookManager::addSymbol(0, "BTC_USDT");
    BookManager::addSymbol(1, "BTC_USD");
    BookManager::addSymbol(2, "ETH_USDT");
    BookManager::addSymbol(3, "ETH_USD");
    BookManager::addSymbol(4, "DOGE_USDT");
    BookManager::addSymbol(5, "DOGE_USD");
    BookManager::addSymbol(6, "PINK_USDT");
    BookManager::addSymbol(7, "PINK_USD");
    auto symHand = BookManager::getSymbolHandler (4);
    symHand.addOrder (0, 10, 3, Side::BUY);
    symHand.addOrder (1, 10, 2, Side::BUY);
    symHand.addOrder (2, 13, 3, Side::BUY);
    symHand.changeOrder (2, 9, 10);
    symHand.addOrder (3, 11, 1, Side::BUY);
    symHand.changeOrder (0, 10, 5);
    symHand.addOrder (4, 3, 10, Side::BUY);
    symHand.removeOrder (3);
    symHand.addOrder (6, 10, 3, Side::SELL);
    symHand.addOrder (7, 10, 3, Side::BUY);
    symHand.removeOrder (7);
    symHand.addOrder (5, 2, 4, Side::BUY);
    int levelnum = 7;
    auto [book, themax] = symHand.queryBookFullInfo (levelnum, Side::BUY);
    cout << "There is/are " << themax << " book level(s) retrieved out of " << levelnum << endl;
    for (int i = 0; i < themax; i++) {
        printBookLevel(book[i]);
    }
}