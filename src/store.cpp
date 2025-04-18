#include "store.h"
#include "debug.h"
#include "buttons.h"

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
    tft.fillRect(0, Menu::menuClearance, tft.width(), tft.height() - Menu::menuClearance, BACKGROUND_COLOR);

    auto bounds = ui.typeText("Store", titleText);
    delay(1000);
    ui.wipeText(bounds);

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.setTextColor(ACCENT_COLOR);
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
        recalcMenuButtons(orderIndex, orders.size());
    }

    tft.fillRect(0, Menu::menuClearance - tft.fontHeight(), tft.width(), tft.height(), BACKGROUND_COLOR);

    const uint16_t startY = Menu::menuClearance + 60;

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);

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
        float progress = (float)index / (size - 1);
        if (progress < 0.1)
        {
            progress = 0.1;
        }

        ledStrip.progress(progress, RgbColor(255 / 4, 94 / 4, 0));
    }
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
        recalcMenuButtons(productIndex, products.size());
    }

    uint16_t y = 60 + 60;
    tft.fillRect(0, Menu::menuClearance - tft.fontHeight(), tft.width(), tft.height(), BACKGROUND_COLOR);

    tft.setFreeFont(&GeistMono_VariableFont_wght14pt7b);
    tft.setTextColor(ACCENT_COLOR);
    tft.setCursor(tft.width() - 20 - tft.textWidth("B") / 2, y);
    tft.print("B");
    tft.setCursor(tft.width() - 20 - tft.textWidth("U") / 2, y + tft.fontHeight() + 4);
    tft.print("U");
    tft.setCursor(tft.width() - 20 - tft.textWidth("Y") / 2, y + tft.fontHeight() * 2 + 8);
    tft.print("Y");

    y += 20;

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);

    tft.setTextColor(ACCENT_COLOR);
    tft.setCursor(20, y);

    auto &product = products[productIndex];
    ui.setIdealFont(product.name.c_str());
    tft.print(product.name.c_str());

    y += 40;
    tft.setCursor(20, y);

    String subheader = product.variants[0].name + " - $" + (product.variants[0].price / 10);
    ui.setIdealFont(subheader.c_str(), nonTitleFonts);
    tft.setTextColor(TEXT_COLOR);
    tft.print(subheader.c_str());
}

void Store::loadProducts()
{
    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = ui.typeText("Loading products...", smallTitleText);
    ui.startBlinking();

#ifdef NO_WIFI
    products = std::vector<Product>{
        Product{"id1", "flow", "coffee", {Variant{"id1", "12oz", 1000}}},
        Product{"id2", "[object Object]", "covfefe", {Variant{"id2", "24oz", 2000}}},
    };
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

void Store::buyProduct()
{
    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(TEXT_COLOR);

    auto tw = tft.textWidth("Hold to buy");
    tft.setCursor(tft.width() / 2 - tw / 2, tft.height() / 2);
    tft.print("Hold to buy");

    const uint16_t initialCircleSize = 20;
    const uint16_t animationDuration = 3000; // 3 seconds total
    const uint16_t maxCircleSize = tft.width() * 2 + initialCircleSize / 2;
    const uint16_t circleX = tft.width() + initialCircleSize / 2;
    const uint16_t circleY = tft.height() / 2 + 40;

    tft.fillCircle(circleX, circleY, initialCircleSize, ACCENT_COLOR);
    delay(1000);

    bool finished = false;
    unsigned long startTime = millis();
    unsigned long elapsedTime = 0;

    uint16_t circleSize = initialCircleSize;

    while (digitalRead(PIN_TERMINAL_BUTTON) == LOW)
    {
        elapsedTime = millis() - startTime;

        float linearProgress = min(1.0f, (float)elapsedTime / animationDuration);
        // Easing function: accelerate towards the end (ease-in-quad)
        float easedProgress = linearProgress * linearProgress;
        circleSize = (easedProgress * maxCircleSize) + initialCircleSize;

        tft.fillCircle(circleX, circleY, circleSize, ACCENT_COLOR);

        auto color = RgbColor::LinearBlend(
            RgbColor(194, 126, 0),
            RgbColor(255, 94, 0), linearProgress);
        ledStrip.progress(linearProgress, color);

        if (elapsedTime >= animationDuration)
        {
            finished = true;
            break;
        }
        delay(10);
    }

    if (!finished)
    {
        ui.menu->taint();
        taint();
        recalcMenuButtons(productIndex, products.size());
        return;
    }

    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = ui.typeText("Clearing cart...", titleText);
    if (terminalApi.clearCart())
    {
        Serial.println("Cart cleared");
    }
    else
    {
        Serial.println("Failed to clear cart");
    }

    ui.wipeText(bounds);
    bounds = ui.typeText("Adding to cart...");
    Cart *cart = terminalApi.addItemToCart(products[productIndex].variants[0].id.c_str(), 1);
    if (!cart)
    {
        taint();
        ui.menu->taint();
        return;
    }

    bounds = ui.typeText(String("Subtotal: $" + String(cart->subtotal / 10)).c_str(), titleText);
    delay(3000);
    ui.wipeText(bounds);

    bounds = ui.typeText("Placing order...", titleText);
    Order *order = terminalApi.convertCartToOrder();
    if (!order)
    {
        taint();
        ui.menu->taint();
        return;
    }

    ui.wipeText(bounds);
    ui.typeText("Order placed", accentText);
    delay(5000);
    ui.wipeText(bounds);

    taint();
    ui.menu->selectMenu(MAIN_MENU);
}

// void order()
// {
//   Serial.println("Fetching products...");
//   std::vector<Product> products = terminalApi.getProducts();
//   Serial.printf("Found %d products\n", products.size());

//   auto bounds = ui.typeText("Product: ");
//   delay(1000);
//   ui.wipeText(bounds);

//   ui.typeText(products[0].name.c_str(), accentText);
//   delay(5000);
//   ui.wipeText(bounds);

//   bounds = ui.typeText("Clearing cart...", titleText);
//   if (terminalApi.clearCart())
//   {
//     Serial.println("Cart cleared");
//   }
//   else
//   {
//     Serial.println("Failed to clear cart");
//   }
//   ui.wipeText(bounds);

//   bounds = ui.typeText("Adding to cart...");
//   Cart *cart = terminalApi.addItemToCart(products[0].variants[0].id.c_str(), 1);
//   if (cart)
//   {
//     Serial.println("Item added to cart");
//     Serial.printf("Subtotal: %d\n", cart->subtotal);
//     Serial.printf("Address ID: %s\n", cart->addressID.c_str());
//     Serial.printf("Card ID: %s\n", cart->cardID.c_str());

//     ui.wipeText(bounds);

//     bounds = ui.typeText("Cart subtotal: ");

//     TextConfig subtotalConfig = ui.createTextConfig(MAIN_FONT);
//     subtotalConfig.y = bounds.y + bounds.height + 8;
//     subtotalConfig.enableCursor = false;
//     auto bounds2 = ui.typeText(String(cart->subtotal).c_str(), subtotalConfig);
//     ui.wipeText(bounds2);
//     ui.wipeText(bounds);

//     delay(5000);
//   }
//   else
//   {
//     Serial.println("Failed to add item to cart");
//   }
//   ui.wipeText(bounds);

//   bounds = ui.typeText("Placing order...");
//   Order *order = terminalApi.convertCartToOrder();
//   if (order)
//   {
//     Serial.println("Order placed");
//     Serial.printf("Order ID: %s\n", order->id.c_str());
//     Serial.printf("Shipping address ID: %s\n", order->shipping.id.c_str());
//     Serial.printf("Tracking number: %s\n", order->tracking.number.c_str());
//     Serial.printf("Tracking URL: %s\n", order->tracking.url.c_str());

//     ui.wipeText(bounds);

//     ui.typeText("Order placed", accentText);

//     delay(5000);
//   }
//   else
//   {
//     Serial.println("Failed to place order");
//   }
//   ui.wipeText(bounds);
// }