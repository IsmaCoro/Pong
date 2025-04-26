🔹 Pong Game en C++ con SFML

📋 Requerimientos del Sistema

✅ Sistema operativo: Windows 64 bits

✅ Compilador: g++ (MinGW.org GCC-6.3.0-1) 6.3.0

✅ Biblioteca gráfica: SFML 2.5.1 - GCC 6.1.0 MinGW (32-bit)

✅ Editor sugerido: Visual Studio Code (o cualquier otro compatible con C++)

✅ Carpeta de recursos: images/ con los archivos:

ball.png

paddle.png

pixelart.ttf

⚙️ Instalación del Compilador MinGW

Descargar el instalador desde SourceForge:👉 https://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version6/gcc-6.3.0

Instalar los siguientes paquetes usando el instalador (mingw-get-setup.exe):

mingw32-base

mingw32-gcc-g++

Agregar el compilador a la variable de entorno:

C:\MinGW\bin

Validar instalación en la terminal (CMD o PowerShell):

g++ --version

✔️ Debería mostrar: g++ (MinGW.org GCC-6.3.0-1) 6.3.0

📦 Instalación de SFML

Descargar SFML 2.5.1 para GCC 6.1.0 MinGW 32-bit:👉 https://www.sfml-dev.org/files/SFML-2.5.1-windows-gcc-6.1.0-mingw-32-bit.zip

Extraer la carpeta y colocarla donde prefieras, por ejemplo:

C:\SFML

Agregar a la variable de entorno:

C:\SFML\bin

Al compilar con g++, incluir y linkear SFML así:

g++ pong.cpp -IC:\SFML\include -LC:\SFML\lib -lsfml-graphics -lsfml-window -lsfml-system -o PongGame.exe

🖼️ Recursos del Juego

Todos los recursos deben estar en una carpeta llamada images dentro del mismo directorio del .exe.

📁 Proyecto
🏻 PongGame.exe
🏻pong.cpp
🏻images
   📷 ball.png
   📷 paddle.png
   📝 pixelart.ttf

En el código, las rutas se verán así:

bolaT.loadFromFile("images/ball.png");
paletaT.loadFromFile("images/paddle.png");
fuente.loadFromFile("images/pixelart.ttf");

✅ Validación final

Verifica que puedes ejecutar:

PongGame.exe

Si ves errores como “Unable to open file”, asegúrate de que la carpeta images/ esté en el mismo nivel que el ejecutable y contenga los recursos correctos.

