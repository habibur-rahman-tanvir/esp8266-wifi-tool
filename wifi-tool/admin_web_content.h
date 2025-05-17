#ifndef ADMIN_WEB_CONTENT_H
#define ADMIN_WEB_CONTENT_H

const char admin_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>Admin Page</title>
        <!-- <link rel="stylesheet" href="./style.css" /> -->
        <link rel="stylesheet" href="/style.css?admin" />
    </head>
    <body>
        <h1>Admin Page</h1>
        <a href="/">Client</a>
        <!-- <script src="./script.js"></script> -->
        <script src="/script.js?admin"></script>
    </body>
</html>
)rawliteral";

const char admin_css[] PROGMEM = R"rawliteral(
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
    background-color: darkgrey;
}
)rawliteral";

const char admin_js[] PROGMEM = R"rawliteral(

"use strict";
console.log("Admin Page");

)rawliteral";

#endif
