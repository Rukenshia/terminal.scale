#include "store.h"
#include "debug.h"

void Store::exit()
{
    reset();
    ledStrip.turnOff();
}

void Store::draw()
{
    if (!tainted)
    {
        return;
    }
    tainted = false;

    if (ui.menu->current == STORE_ORDERS)
    {
        drawOrders();
        return;
    }
    else if (ui.menu->current == STORE_BROWSE)
    {
        drawProducts();
        return;
    }

    // clear the screen except menu clearance
    const uint16_t menuClearanceY = 60;
    tft.fillRect(0, menuClearanceY, tft.width(), tft.height() - menuClearanceY, BACKGROUND_COLOR);

    auto bounds = ui.typeText("Store", titleText);
    delay(1000);
    ui.wipeText(bounds);

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.setTextColor(ACCENT_COLOR);

    const uint16_t startY = menuClearanceY + 40;

    Serial.printf("Drawing store at %d\n", startY);

    tft.setCursor(tft.width() - 20 - tft.textWidth("B") / 2, startY);
    tft.print("B");
    tft.setCursor(tft.width() - 20 - tft.textWidth("U") / 2, startY + tft.fontHeight() + 4);
    tft.print("U");
    tft.setCursor(tft.width() - 20 - tft.textWidth("Y") / 2, startY + tft.fontHeight() * 2 + 8);
    tft.print("Y");
}

void Store::loadOrders()
{
    if (ordersLoaded)
    {
        Serial.println("Orders already loaded");
        return;
    }

    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = ui.typeText("Loading orders...", smallTitleText);
    ui.startBlinking();

#ifdef NO_WIFI
    orders = {
        {"id1", "2025-01-01T12:34:56Z", 1, {"John Doe", "123 Main St", "", "New York", "NY", "10001", "USA", "555-1234"}, {1000, 50}, {"UPS", "1234567890", "SHIPPED", "http://example.com/track"}},
        {"id2", "2024-12-20T34:56:00Z", 2, {"Jane Smith", "456 Elm St", "", "Los Angeles", "CA", "90001", "USA", "555-5678"}, {2000, 100}, {"FedEx", "0987654321", "DELIVERED", "http://example.com/track"}},
    };
#else
    orders = terminalApi.getOrders();
#endif
    ordersLoaded = true;

    ui.wipeText(bounds);
    ui.stopBlinking();

    if (orders.empty())
    {
        ui.typeText("No orders found", smallTitleText);
        delay(2000);

        ui.menu->selectMenu(STORE);
        return;
    }

    recalcMenuButtons(orderIndex, orders.size());
    ui.menu->taint();
}

void Store::drawOrders()
{
    if (!ordersLoaded)
    {
        loadOrders();
    }

    const uint16_t startY = 60 + 60;

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.fillRect(0, startY - tft.fontHeight(), tft.width(), tft.height(), BACKGROUND_COLOR);

    tft.setTextColor(ACCENT_COLOR);
    tft.setCursor(20, startY);

    auto &order = orders[orderIndex];
    tft.print(order.id.c_str());

    String subheader = order.created.substring(10) + " - " + order.tracking.status;
    tft.setCursor(20, startY + tft.fontHeight() + 4);
    tft.setFreeFont(&GeistMono_VariableFont_wght12pt7b);
    tft.setTextColor(TEXT_COLOR);
    tft.print(subheader.c_str());
}

void Store::recalcMenuButtons(int index, int size)
{
    bool shouldRedraw = false;

    if (index == size - 1)
    {
        ui.menu->hideButton(MenuButton::RIGHT);
        shouldRedraw = true;
    }
    else if (!ui.menu->isButtonVisible(MenuButton::RIGHT))
    {
        ui.menu->showButton(MenuButton::RIGHT);
        shouldRedraw = true;
    }

    if (index == 0)
    {
        ui.menu->hideButton(MenuButton::LEFT);
        shouldRedraw = true;
    }
    else if (!ui.menu->isButtonVisible(MenuButton::LEFT))
    {
        ui.menu->showButton(MenuButton::LEFT);
        shouldRedraw = true;
    }

    if (shouldRedraw)
    {
        ui.menu->redraw();
    }

    // Update LED Strip to show the progress (how far have we scrolled through the list of orders)
    ledStrip.progress(index / size);
}

void Store::nextOrder()
{
    orderIndex++;
    if (orderIndex >= orders.size())
    {
        orderIndex--;
    }

    recalcMenuButtons(orderIndex, orders.size());
    taint();
}

void Store::previousOrder()
{
    if (orderIndex == 0)
    {
        return;
    }

    orderIndex--;
    recalcMenuButtons(orderIndex, orders.size());
    taint();
}

void Store::drawProducts()
{
    if (!productsLoaded)
    {
        loadProducts();
    }

    const uint16_t startY = 60 + 60;

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.fillRect(0, startY - tft.fontHeight(), tft.width(), tft.height(), BACKGROUND_COLOR);

    tft.setTextColor(ACCENT_COLOR);
    tft.setCursor(20, startY);

    auto &product = products[productIndex];
    tft.print(product.id.c_str());

    String subheader = product.variants[0].name + " - $" + (product.variants[0].price / 10);
    tft.setCursor(20, startY + tft.fontHeight() + 4);
    tft.setFreeFont(&GeistMono_VariableFont_wght12pt7b);
    tft.setTextColor(TEXT_COLOR);
    tft.print(subheader.c_str());
}

void Store::loadProducts()
{
    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = ui.typeText("Loading products...", smallTitleText);
    ui.startBlinking();

#ifdef NO_WIFI
    products = {};
#else
    products = terminalApi.getProducts();
#endif
    productsLoaded = true;

    ui.wipeText(bounds);
    ui.stopBlinking();

    if (products.empty())
    {
        ui.typeText("No products found", smallTitleText);
        delay(2000);

        ui.menu->selectMenu(STORE);
        return;
    }

    ui.menu->taint();
}

void Store::previousProduct()
{
    if (productIndex == 0)
    {
        return;
    }

    productIndex--;

    recalcMenuButtons(productIndex, products.size());
    taint();
}

void Store::nextProduct()
{
    if (productIndex >= products.size() - 1)
    {
        return;
    }

    productIndex++;

    recalcMenuButtons(productIndex, products.size());
    taint();
}