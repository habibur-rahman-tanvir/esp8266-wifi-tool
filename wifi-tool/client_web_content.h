#ifndef CLIENT_WEB_CONTENT_H
#define CLIENT_WEB_CONTENT_H

const char client_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <title>Client Page</title>
        <!-- <link rel="stylesheet" href="./style.css" /> -->
        <link rel="stylesheet" href="/style.css">
    </head>
    <body>
        <h1>Welcome, Client!</h1>
        <a href="/admin">Admin</a>
        <!-- <script src="./script.js"></script> -->
        <script src="/script.js"></script>
    </body>
</html>
)rawliteral";

const char client_css[] PROGMEM = R"rawliteral(
* {
    margin: 0;
    box-sizing: border-box;
    text-decoration: none;
    border: none;
}
html {
    font-size: 62.5%;
}
body {
    background-color: rgb(53, 65, 159);
}
)rawliteral";

const char client_js[] PROGMEM = R"rawliteral(
console.log("Client Pages");
)rawliteral";

#endif
