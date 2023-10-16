#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "POCO-F3";
const char *password = "Jose Rizo";

WebServer server(80);

const int accountId1 = 12345; // ID de cuenta existente
const int accountId2 = 67890; // ID de cuenta existente

float accountBalance1 = 1000.0; // Saldo de cuenta 1
float accountBalance2 = 500.0;  // Saldo de cuenta 2

void handleRoot()
{
    String html = "<html><body><h1>Buscar Cuenta</h1>";
    html += "<form action='/search' method='get'>";
    html += "ID de Cuenta: <input type='text' name='accountId'><br>";
    html += "<input type='submit' value='Buscar'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

void handleSearch()
{
    String accountIdStr = server.arg("accountId");
    int accountId = accountIdStr.toInt();

    String response;
    if (accountId == accountId1)
    {
        response = "Número de Cuenta: 9876543210, Dinero: " + String(accountBalance1);
    }
    else if (accountId == accountId2)
    {
        response = "Número de Cuenta: 5432109876, Dinero: " + String(accountBalance2);
    }
    else
    {
        response = "La cuenta no existe.";
    }

    server.send(200, "text/html", response);
}

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Conectando a la red WiFi...");
    }

    Serial.println("Conexión WiFi exitosa");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/search", HTTP_GET, handleSearch);

    server.begin();
    Serial.println("Servidor iniciado");
}

void loop()
{
    server.handleClient();
}
