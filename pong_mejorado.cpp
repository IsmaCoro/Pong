#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>

using namespace sf;
using namespace std;

// Enumeraciones para los estados del juego
enum GameState
{
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};
enum AILevel
{
    EASY,
    MEDIUM,
    HARD,
    IMPOSSIBLE
};
enum GameMode
{
    PLAYER_VS_AI,
    PLAYER_VS_PLAYER,
    AI_VS_AI
};

// Enumeraciones para los tipos de power-ups
enum PowerUpType
{
    BIGGER_PADDLE,
    SMALLER_OPPONENT,
    SLOW_BALL,
    DOUBLE_BALL,
    BARRIER,
    INVERT_CONTROLS,
    FLASHING_BALL,
    DOUBLE_POINTS,
    LESS_POINTS,
    FREEZE_OPPONENT,
    INVISIBLE_OPPONENT
};

// Clase para la pelota
class Ball
{
private:
    Sprite sprite;
    Vector2f velocity;
    float baseSpeed;
    float maxSpeed;
    bool active;
    bool visible;
    bool isFlashing;
    Clock flashTimer;
    float flashDuration; // segundos
    float flashInterval; // segundos entre cambios de visibilida

public:
    Ball(Texture &texture) : isFlashing(false), flashDuration(3.0f), flashInterval(0.3f), visible(true)
    {
        sprite.setTexture(texture);
        sprite.setOrigin((float)texture.getSize().x / 2, (float)texture.getSize().y / 2);
        sprite.setScale(0.25f, 0.25f);
        reset();
        baseSpeed = 3.0f;
        maxSpeed = 8.0f;
        active = true;
    }

    void startFlashing(float duration)
    {
        isFlashing = true;
        flashDuration = duration;
        flashTimer.restart();
        setVisible(true); // Asegurarse que empiece visible
    }

    void updateFlashing()
    {
        if (isFlashing)
        {
            if (flashTimer.getElapsedTime().asSeconds() >= flashDuration)
            {
                isFlashing = false;
                setVisible(true); // Al terminar el efecto, volver a ser visible
            }
            else
            {
                float elapsed = flashTimer.getElapsedTime().asSeconds();
                bool shouldBeVisible = static_cast<int>(elapsed / flashInterval) % 2 == 0;
                setVisible(shouldBeVisible);
            }
        }
    }

    void reset()
    {
        sprite.setPosition(425, 285); // Ajustado para el nuevo área de juego
        // Velocidad inicial aleatoria
        float angle = (rand() % 60 - 30) * 3.14159f / 180.0f;
        velocity.x = baseSpeed * cos(angle);
        velocity.y = baseSpeed * sin(angle);

        // Asegurar que la pelota vaya hacia un lado aleatorio
        if (rand() % 2 == 0)
        {
            velocity.x = -velocity.x;
        }
    }

    void update()
    {
        if (!active)
            return; // Solo si la pelota ya fue destruida del juego
        sprite.move(velocity);
    }

    void accelerate()
    {
        // Aumentar velocidad en un 5%
        float currentSpeed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (currentSpeed < maxSpeed)
        {
            float factor = min(currentSpeed * 1.05f, maxSpeed) / currentSpeed;
            velocity.x *= factor;
            velocity.y *= factor;
        }
    }

    void slowDown(float factor)
    {
        velocity.x *= factor;
        velocity.y *= factor;
    }

    void reverseX() { velocity.x = -velocity.x; }
    void reverseY() { velocity.y = -velocity.y; }

    void setVisible(bool state) { visible = state; }
    bool isVisible() const { return visible; }

    // En la clase Ball (línea ~84)
    Sprite &getSprite() { return sprite; }
    const Sprite &getSprite() const { return sprite; }
    Vector2f getPosition() const { return sprite.getPosition(); }
    Vector2f getVelocity() const { return velocity; }
    void setActive(bool state) { active = state; }
    bool isActive() const { return active; }
};

// Clase para la paleta
class Paddle
{
private:
    Sprite sprite;
    float speed;
    float originalScale;
    bool invertedControls;
    AILevel aiLevel;
    bool isAI;

public:
    Paddle(Texture &texture, bool isLeftPaddle, bool isAIControlled = false, AILevel level = EASY)
    {
        sprite.setTexture(texture);
        sprite.setOrigin((float)texture.getSize().x / 2, (float)texture.getSize().y / 2);

        if (isLeftPaddle)
        {
            sprite.setRotation(90);
            sprite.setPosition(25, 250);
        }
        else
        {
            sprite.setRotation(-90);
            sprite.setPosition(825, 250);
        }

        speed = 4.0f;
        originalScale = 1.0f;
        invertedControls = false;
        isAI = isAIControlled;
        aiLevel = level;
    }

    void update(const vector<Ball> &balls, bool isLeftPaddle)
    {
        if (isAI)
        {
            updateAI(balls, isLeftPaddle);
        }
    }

    void updateAI(const vector<Ball> &balls, bool isLeftPaddle)
    {
        // No hacer nada si no hay pelotas activas
        if (balls.empty())
            return;

        // Encontrar la pelota más cercana que se dirige hacia esta paleta
        Ball *targetBall = nullptr;
        float closestDistance = 1000000.0f;

        for (const Ball &ball : balls)
        {
            if (!ball.isActive())
                continue;

            Vector2f ballPos = ball.getPosition();
            Vector2f ballVel = ball.getVelocity();

            // Solo considerar pelotas que vienen hacia esta paleta
            if ((isLeftPaddle && ballVel.x < 0) || (!isLeftPaddle && ballVel.x > 0))
            {
                float distance = abs(ballPos.x - sprite.getPosition().x);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    targetBall = const_cast<Ball *>(&ball);
                }
            }
        }

        if (targetBall == nullptr)
        {
            // Si no hay pelotas viniendo, volver al centro
            moveTowardsY(250);
            return;
        }

        Vector2f ballPos = targetBall->getPosition();
        Vector2f ballVel = targetBall->getVelocity();

        // Calcular dónde estará la pelota cuando llegue a la posición X de la paleta
        float timeToReach = abs((sprite.getPosition().x - ballPos.x) / ballVel.x);
        float predictedY = ballPos.y + ballVel.y * timeToReach;

        // Ajustar por rebotes en las paredes
        while (predictedY < 70 || predictedY > 550)
        {
            if (predictedY < 70)
                predictedY = 140 - predictedY;
            if (predictedY > 550)
                predictedY = 1100 - predictedY;
        }

        // Añadir error según el nivel de dificultad
        float errorChance = 0.0f;
        float speedFactor = 1.0f;
        float errorAmount = 0.0f;

        switch (aiLevel)
        {
        case EASY:
            errorChance = 0.4f;
            speedFactor = 0.6f;
            errorAmount = 100.0f;
            break;
        case MEDIUM:
            errorChance = 0.2f;
            speedFactor = 0.75f;
            errorAmount = 50.0f;
            break;
        case HARD:
            errorChance = 0.05f;
            speedFactor = 0.9f;
            errorAmount = 20.0f;
            break;
        case IMPOSSIBLE:
            errorChance = 0.0f;
            speedFactor = 1.0f;
            errorAmount = 0.0f;
            break;
        }

        // Aplicar error aleatorio
        if ((float)rand() / RAND_MAX < errorChance)
        {
            predictedY += (rand() % (int)(errorAmount * 2) - errorAmount);
        }

        // Mover hacia la posición predicha
        moveTowardsY(predictedY, speedFactor);
    }

    void moveTowardsY(float targetY, float speedFactor = 1.0f)
    {
        float actualSpeed = speed * speedFactor;
        float currentY = sprite.getPosition().y;

        if (abs(currentY - targetY) < actualSpeed)
        {
            sprite.setPosition(sprite.getPosition().x, targetY);
        }
        else if (currentY < targetY)
        {
            sprite.move(0, actualSpeed);
        }
        else
        {
            sprite.move(0, -actualSpeed);
        }

        // Asegurar que la paleta no salga de la pantalla
        Vector2f pos = sprite.getPosition();
        if (pos.y < 70 + sprite.getGlobalBounds().height / 2)
            sprite.setPosition(pos.x, 70 + sprite.getGlobalBounds().height / 2);
        if (pos.y > 550 - sprite.getGlobalBounds().height / 2)
            sprite.setPosition(pos.x, 550 - sprite.getGlobalBounds().height / 2);
    }

    void move(float offsetY)
    {
        // Mueve la paleta
        sprite.move(0, offsetY);

        // Obtener la posición actual
        Vector2f pos = sprite.getPosition();

        // Verificar y ajustar si la paleta se sale de los límites
        if (pos.y < 70 + sprite.getGlobalBounds().height / 2)
        {
            sprite.setPosition(pos.x, 70 + sprite.getGlobalBounds().height / 2);
        }
        else if (pos.y > 550 - sprite.getGlobalBounds().height / 2)
        {
            sprite.setPosition(pos.x, 550 - sprite.getGlobalBounds().height / 2);
        }
    }

    void setSize(float scaleFactor)
    {
        if (sprite.getRotation() == 90 || sprite.getRotation() == -90)
        {
            // Para paletas rotadas 90 o -90 grados, ajustamos la escala X
            sprite.setScale(originalScale * scaleFactor, sprite.getScale().y);
        }
        else
        {
            // Para paletas sin rotación, ajustamos la escala Y
            sprite.setScale(sprite.getScale().x, originalScale * scaleFactor);
        }
    }

    void resetSize()
    {
        // Restaurar la escala original en ambos ejes
        sprite.setScale(originalScale, originalScale);
    }

    void setInvertedControls(bool inverted)
    {
        invertedControls = inverted;
    }

    void setAILevel(AILevel level)
    {
        aiLevel = level;
    }

    Sprite &getSprite() { return sprite; }
    const Sprite &getSprite() const { return sprite; }
    bool hasInvertedControls() { return invertedControls; }
    AILevel getAILevel() { return aiLevel; }
    void setIsAI(bool ai) { isAI = ai; }
    bool getIsAI() { return isAI; }
    float getSpeed() { return speed; } // Añadir este método
};

// Clase para los power-ups
class PowerUp
{
private:
    PowerUpType type;
    Sprite sprite;
    Clock timer;
    bool active;
    bool collected;
    int duration; // en segundos

public:
    PowerUp(PowerUpType t, Texture &texture)
    {
        type = t;
        sprite.setTexture(texture);
        sprite.setOrigin((float)texture.getSize().x / 2, (float)texture.getSize().y / 2);
        sprite.setScale(0.5f, 0.5f);

        // Posición aleatoria en el campo
        float x = 100 + rand() % 650;
        float y = 120 + rand() % 380; // Ajustado para el nuevo área de juego
        sprite.setPosition(x, y);

        active = true;
        collected = false;
        duration = 5; // 5 segundos por defecto
    }

    void update()
    {
        if (collected && timer.getElapsedTime().asSeconds() >= duration)
        {
            active = false;
        }
    }

    void collect()
    {
        collected = true;
        timer.restart();
    }

    PowerUpType getType() const { return type; }
    const Sprite &getSprite() const { return sprite; }
    bool isActive() const { return active; }
    bool isCollected() const { return collected; }
};

// Clase para el temporizador
class GameTimer
{
private:
    Clock clock;
    int totalSeconds;
    Text display;

public:
    GameTimer(Font &font, int minutes)
    {
        totalSeconds = minutes * 60;
        display.setFont(font);
        display.setCharacterSize(30);
        display.setPosition(425, 30); // Centrado en la parte superior
        display.setOrigin(display.getLocalBounds().width / 2, 0);
        updateDisplay();
    }

    void updateDisplay()
    {
        int remainingSeconds = totalSeconds - (int)clock.getElapsedTime().asSeconds();
        if (remainingSeconds < 0)
            remainingSeconds = 0;

        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;

        string timeStr = (minutes < 10 ? "0" : "") + to_string(minutes) + ":" +
                         (seconds < 10 ? "0" : "") + to_string(seconds);
        display.setString(timeStr);

        // Actualizar el origen para mantenerlo centrado cuando cambia el texto
        display.setOrigin(display.getLocalBounds().width / 2, 0);
    }

    bool isTimeUp()
    {
        return clock.getElapsedTime().asSeconds() >= totalSeconds;
    }

    void reset()
    {
        clock.restart();
    }

    Text &getDisplay() { return display; }
};

// Clase para el menú
class Menu
{
private:
    vector<Text> options;
    int selectedOption;
    Font &font;

    // Configuraciones del juego
    GameMode gameMode;
    AILevel aiLevel1;
    AILevel aiLevel2;
    int gameDuration; // en minutos
    int maxScore;
    bool powerUpsEnabled;
    float initialBallSpeed;

public:
    Menu(Font &f) : font(f)
    {
        selectedOption = 0;

        // Valores por defecto
        gameMode = PLAYER_VS_AI;
        aiLevel1 = MEDIUM;
        aiLevel2 = MEDIUM;
        gameDuration = 3;
        maxScore = 7;
        powerUpsEnabled = true;
        initialBallSpeed = 3.0f;

        // Crear opciones del menú
        createMenuOptions();
    }

    void createMenuOptions()
    {
        options.clear();

        // Título
        Text title("PONG 2.0", font, 50);

        // Centrar el texto horizontalmente
        FloatRect textBounds = title.getLocalBounds();
        title.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top); // Centra horizontalmente el origen
        title.setPosition(850 / 2.0f, 50);                                          // Posición centrada en X, Y fija

        options.push_back(title);

        // Opciones principales
        addOption("Player vs IA", 200);
        addOption("2 Player", 250);
        addOption("IA vs IA", 300);
        addOption("Opciones", 350);
        addOption("Salir", 400);

        // Resaltar la opción seleccionada
        options[selectedOption + 1].setFillColor(Color::Yellow);
    }

    void addOption(const std::string &text, float y)
    {
        Text option(text, font, 30);
        FloatRect bounds = option.getLocalBounds();
        option.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top); // Centra horizontalmente
        option.setPosition(850 / 2.0f, y);                               // Centra en X
        options.push_back(option);
    }

    void moveUp()
    {
        options[selectedOption + 1].setFillColor(Color::White);
        selectedOption = (selectedOption - 1 + 5) % 5; // 5 opciones en total
        options[selectedOption + 1].setFillColor(Color::Yellow);
    }

    void moveDown()
    {
        options[selectedOption + 1].setFillColor(Color::White);
        selectedOption = (selectedOption + 1) % 5; // 5 opciones en total
        options[selectedOption + 1].setFillColor(Color::Yellow);
    }

    int getSelectedOption()
    {
        return selectedOption;
    }

    void draw(RenderWindow &window)
    {
        for (const auto &option : options)
        {
            window.draw(option);
        }
    }

    // Getters y setters para las configuraciones
    GameMode getGameMode() { return gameMode; }
    void setGameMode(GameMode mode) { gameMode = mode; }

    AILevel getAILevel1() { return aiLevel1; }
    void setAILevel1(AILevel level) { aiLevel1 = level; }

    AILevel getAILevel2() { return aiLevel2; }
    void setAILevel2(AILevel level) { aiLevel2 = level; }

    int getGameDuration() { return gameDuration; }
    void setGameDuration(int minutes) { gameDuration = minutes; }

    int getMaxScore() { return maxScore; }
    void setMaxScore(int score) { maxScore = score; }

    bool arePowerUpsEnabled() { return powerUpsEnabled; }
    void setPowerUpsEnabled(bool enabled) { powerUpsEnabled = enabled; }

    float getInitialBallSpeed() { return initialBallSpeed; }
    void setInitialBallSpeed(float speed) { initialBallSpeed = speed; }
};

// Clase principal del juego
class Game
{
private:
    RenderWindow window;
    GameState state;

    // Recursos
    Texture ballTexture;
    Texture paddleTexture;
    Texture powerUpTextures[11]; // Una textura para cada tipo de power-up
    Font font;

    // Elementos del juego
    vector<Ball> balls;
    Paddle leftPaddle = Paddle(paddleTexture, true, false, EASY);
    Paddle rightPaddle = Paddle(paddleTexture, false, true, EASY); // Inicializar con un valor por defecto
    vector<PowerUp> powerUps;
    bool freezeLeftActive;
    bool freezeRightActive;
    Clock freezeTimerLeft;
    Clock freezeTimerRight;
    bool invisibleLeftActive;
    bool invisibleRightActive;
    Clock invisibleTimerLeft;
    Clock invisibleTimerRight;
    bool biggerLeftActive;
    bool biggerRightActive;
    Clock biggerTimerLeft;
    Clock biggerTimerRight;
    bool barrierLeftActive;
    bool barrierRightActive;
    Clock barrierTimerLeft;
    Clock barrierTimerRight;
    RectangleShape leftBarrier;
    RectangleShape rightBarrier;
    bool smallerLeftActive;
    bool smallerRightActive;
    Clock smallerTimerLeft;
    Clock smallerTimerRight;
    Clock doublePointsTimer;
    Clock lessPointsTimer;

    // Interfaz
    Text scoreLeft;
    Text scoreRight;
    Text pauseText;
    Text gameOverText;
    RectangleShape headerBar; // Barra para separar el área de puntaje del juego

    // Lógica del juego
    int leftScore;
    int rightScore;
    GameTimer *timer;
    Menu *menu;
    Clock powerUpSpawnTimer;
    bool doublePointsActive;
    bool lessPointsActive;

    // Configuraciones
    GameMode gameMode;
    int maxScore;
    bool powerUpsEnabled;

public:
    Game() : window(VideoMode(850, 550), "Pong 2.0") // Aumentar altura para el área de puntaje
    {
        // Inicializar el generador de números aleatorios
        srand(static_cast<unsigned int>(time(nullptr)));

        // Cargar recursos ANTES de crear las paletas
        if (!ballTexture.loadFromFile("c:\\Pong\\imagesBri\\Pelota.png"))
        {
            cout << "Error al cargar textura Bola" << endl;
        }

        if (!paddleTexture.loadFromFile("c:\\Pong\\imagesBri\\Paleta.png"))
        {
            cout << "Error al cargar textura Paleta" << endl;
        }

        if (!font.loadFromFile("c:\\Pong\\images\\pixelart.ttf"))
        {
            cout << "Error al cargar Fuente Pixel Art" << endl;
        }

        // Inicializar las paletas DESPUÉS de cargar las texturas
        leftPaddle = Paddle(paddleTexture, true, false, EASY);
        rightPaddle = Paddle(paddleTexture, false, true, menu->getAILevel1());

        // Cargar texturas de power-ups (esto es un placeholder, necesitarías crear estas imágenes)
        // En una implementación real, cargarías imágenes distintas para cada power-up
        if (!powerUpTextures[BIGGER_PADDLE].loadFromFile("c:\\Pong\\imagesBri\\PaletaMasGrande.png"))
        {
            cout << "Error al cargar textura para BIGGER_PADDLE" << endl;
        }
        if (!powerUpTextures[SMALLER_OPPONENT].loadFromFile("c:\\Pong\\imagesBri\\PaletaMasPequena.png"))
        {
            cout << "Error al cargar textura para SMALLER_OPPONENT" << endl;
        }
        if (!powerUpTextures[SLOW_BALL].loadFromFile("c:\\Pong\\imagesBri\\pelotaLenta.png"))
        {
            cout << "Error al cargar textura para SLOW_BALL" << endl;
        }
        if (!powerUpTextures[DOUBLE_BALL].loadFromFile("c:\\Pong\\imagesBri\\DoblePelota.png"))
        {
            cout << "Error al cargar textura para DOUBLE_BALL" << endl;
        }
        if (!powerUpTextures[BARRIER].loadFromFile("c:\\Pong\\imagesBri\\Barrera.png"))
        {
            cout << "Error al cargar textura para BARRIER" << endl;
        }
        if (!powerUpTextures[INVERT_CONTROLS].loadFromFile("c:\\Pong\\imagesBri\\CambioDeControles.png"))
        {
            cout << "Error al cargar textura para INVERT_CONTROLS" << endl;
        }
        if (!powerUpTextures[FLASHING_BALL].loadFromFile("c:\\Pong\\imagesBri\\PelotasFantasmas.png"))
        {
            cout << "Error al cargar textura para FLASHING_BALL" << endl;
        }
        if (!powerUpTextures[DOUBLE_POINTS].loadFromFile("c:\\Pong\\imagesBri\\PuntosDobles.png"))
        {
            cout << "Error al cargar textura para DOUBLE_POINTS" << endl;
        }
        if (!powerUpTextures[LESS_POINTS].loadFromFile("c:\\Pong\\imagesBri\\PuntosNegativos.png"))
        {
            cout << "Error al cargar textura para LESS_POINTS" << endl;
        }
        if (!powerUpTextures[FREEZE_OPPONENT].loadFromFile("c:\\Pong\\imagesBri\\BloqueoPaleta.png"))
        {
            cout << "Error al cargar textura para FREEZE_OPPONENT" << endl;
        }
        if (!powerUpTextures[INVISIBLE_OPPONENT].loadFromFile("c:\\Pong\\imagesBri\\VisionObstruida.png"))
        {
            cout << "Error al cargar textura para INVISIBLE_OPPONENT" << endl;
        }

        // Configurar la ventana
        window.setFramerateLimit(120);

        // Inicializar el estado del juego
        state = MENU;

        // Crear el menú
        menu = new Menu(font);

        // Actualizar el nivel de AI de la paleta derecha
        rightPaddle = Paddle(paddleTexture, false, true, menu->getAILevel1());

        // Inicializar la pelota
        balls.push_back(Ball(ballTexture));

        // Configurar el texto
        scoreLeft.setFont(font);
        scoreLeft.setCharacterSize(40);
        scoreLeft.setPosition(200, 30); // Posición en la barra superior

        scoreRight.setFont(font);
        scoreRight.setCharacterSize(40);
        scoreRight.setPosition(650, 30); // Posición en la barra superior

        // Crear barra de separación
        headerBar.setSize(Vector2f(850, 70));      // Altura de la barra superior
        headerBar.setFillColor(Color(20, 20, 20)); // Color ligeramente diferente al fondo
        headerBar.setPosition(0, 0);

        pauseText.setFont(font);
        pauseText.setCharacterSize(50);
        pauseText.setString("PAUSA");
        pauseText.setPosition(350, 200);
        pauseText.setFillColor(Color::White);

        gameOverText.setFont(font);
        gameOverText.setCharacterSize(30); // Tamaño más pequeño
        gameOverText.setFillColor(Color::White);

        // Centrar el texto horizontalmente
        FloatRect textBounds = gameOverText.getLocalBounds();
        gameOverText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        gameOverText.setPosition(425, 275); // Centro de la pantalla

        // Inicializar la lógica del juego
        leftScore = 0;
        rightScore = 0;
        doublePointsActive = false;
        lessPointsActive = false;
        freezeLeftActive = false;
        freezeRightActive = false;
        invisibleLeftActive = false;
        invisibleRightActive = false;
        biggerLeftActive = false;
        biggerRightActive = false;
        barrierLeftActive = false;
        barrierRightActive = false;
        smallerLeftActive = false;
        smallerRightActive = false;

        // Configurar las barreras
        leftBarrier.setSize(Vector2f(10, 100));
        leftBarrier.setFillColor(Color(255, 0, 0, 128)); // Rojo semi-transparente
        leftBarrier.setPosition(100, 225);

        rightBarrier.setSize(Vector2f(10, 100));
        rightBarrier.setFillColor(Color(255, 0, 0, 128)); // Rojo semi-transparente
        rightBarrier.setPosition(740, 225);

        rightBarrier.setSize(Vector2f(10, 100));
        rightBarrier.setFillColor(Color(255, 0, 0, 128)); // Rojo semi-transparente
        rightBarrier.setPosition(740, 225);
        updateScoreDisplay();

        // Crear el temporizador (3 minutos por defecto)
        timer = new GameTimer(font, 3);

        // Configuraciones por defecto
        gameMode = PLAYER_VS_AI;
        maxScore = 7;
        powerUpsEnabled = true;
    }

    ~Game()
    {
        delete timer;
        delete menu;
    }

    void run()
    {
        while (window.isOpen())
        {
            handleEvents();
            update();
            render();
        }
    }

private:
    void handleEvents()
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::KeyPressed)
            {
                if (state == MENU)
                {
                    handleMenuInput(event.key.code);
                }
                else if (state == PLAYING)
                {
                    if (event.key.code == Keyboard::Escape)
                    {
                        state = PAUSED;
                    }
                }
                else if (state == PAUSED)
                {
                    if (event.key.code == Keyboard::Escape)
                    {
                        state = PLAYING;
                    }
                    else if (event.key.code == Keyboard::R)
                    {
                        resetGame();
                        state = PLAYING;
                    }
                    else if (event.key.code == Keyboard::M)
                    {
                        state = MENU;
                    }
                }
                else if (state == GAME_OVER)
                {
                    if (event.key.code == Keyboard::R)
                    {
                        resetGame();
                        state = PLAYING;
                    }
                    else if (event.key.code == Keyboard::M)
                    {
                        state = MENU;
                    }
                }
            }
        }
    }

    void handleMenuInput(Keyboard::Key key)
    {
        if (key == Keyboard::Up)
        {
            menu->moveUp();
        }
        else if (key == Keyboard::Down)
        {
            menu->moveDown();
        }
        else if (key == Keyboard::Return)
        {
            int option = menu->getSelectedOption();
            switch (option)
            {
            case 0: // Un Jugador vs IA
                // En lugar de iniciar el juego directamente, mostrar submenú de dificultad
                showAIDifficultyMenu();
                break;
            case 1: // Dos Jugadores
                gameMode = PLAYER_VS_PLAYER;
                leftPaddle.setIsAI(false);
                rightPaddle.setIsAI(false);
                resetGame();
                state = PLAYING;
                break;
            case 2: // IA vs IA
                gameMode = AI_VS_AI;
                leftPaddle.setIsAI(true);
                rightPaddle.setIsAI(true);
                leftPaddle.setAILevel(menu->getAILevel1());
                rightPaddle.setAILevel(menu->getAILevel2());
                resetGame();
                state = PLAYING;
                break;
            case 3: // Opciones
                // Mostrar menú de opciones
                showOptionsMenu();
                break;
            case 4: // Salir
                window.close();
                break;
            }
        }
    }

    void update()
    {
        if (state == PLAYING)
        {
            // Actualizar temporizador
            timer->updateDisplay();

            // Verificar condiciones de fin de juego
            if (timer->isTimeUp() || leftScore >= maxScore || rightScore >= maxScore)
            {
                // Configurar el texto de game over
                gameOverText.setFont(font);
                gameOverText.setString(leftScore > rightScore ? "JUGADOR 1 GANA!" : (rightScore > leftScore ? "JUGADOR 2 GANA!" : "EMPATE!"));
                gameOverText.setCharacterSize(30); // Tamaño más pequeño

                // Centrar el texto horizontalmente y verticalmente
                FloatRect textBounds = gameOverText.getLocalBounds();
                gameOverText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
                gameOverText.setPosition(425, 200); // Posicionado más arriba en la pantalla

                state = GAME_OVER;
                return;
            }

            // Verificar si los efectos de power-up han expirado
            if (freezeLeftActive && freezeTimerLeft.getElapsedTime().asSeconds() >= 5.0f)
            {
                freezeLeftActive = false;
            }

            if (freezeRightActive && freezeTimerRight.getElapsedTime().asSeconds() >= 5.0f)
            {
                freezeRightActive = false;
            }
            
            if (doublePointsActive && doublePointsTimer.getElapsedTime().asSeconds() >= 5.0f)
            {
                doublePointsActive = false;
            }

            if (lessPointsActive && lessPointsTimer.getElapsedTime().asSeconds() >= 5.0f)
            {
                lessPointsActive = false;
            }

            // Verificar si el efecto de barrera ha expirado
            if (barrierLeftActive && barrierTimerLeft.getElapsedTime().asSeconds() >= 5.0f)
            {
                barrierLeftActive = false;
            }

            if (barrierRightActive && barrierTimerRight.getElapsedTime().asSeconds() >= 5.0f)
            {
                barrierRightActive = false;
            }

            // Verificar si el efecto de paleta más pequeña ha expirado
            if (smallerLeftActive && smallerTimerLeft.getElapsedTime().asSeconds() >= 5.0f)
            {
                smallerLeftActive = false;
                leftPaddle.resetSize();
            }

            if (smallerRightActive && smallerTimerRight.getElapsedTime().asSeconds() >= 5.0f)
            {
                smallerRightActive = false;
                rightPaddle.resetSize();
            }

            // Actualizar pelotas
            updateBalls();

            // Actualizar paletas
            updatePaddles();

            // Manejar colisiones
            handleCollisions();

            // Generar power-ups
            if (powerUpsEnabled && powerUpSpawnTimer.getElapsedTime().asSeconds() > 10)
            { // Cada 10 segundos
                spawnPowerUp();
                powerUpSpawnTimer.restart();
            }

            // Actualizar power-ups
            updatePowerUps();
        }
    }

    void updateBalls()
    {
        bool goalScored = false;

        // Actualizar posición de las pelotas
        for (auto &ball : balls)
        {
            ball.updateFlashing(); // Actualizar estado de parpadeo

            if (!ball.isActive())
                continue;

            ball.update();

            // Comprobar colisiones con las paletas
            if (rightPaddle.getSprite().getGlobalBounds().contains(ball.getPosition()))
            {
                ball.reverseX();
                ball.accelerate();
            }
            else if (leftPaddle.getSprite().getGlobalBounds().contains(ball.getPosition()))
            {
                ball.reverseX();
                ball.accelerate();
            }
            // Comprobar colisiones con las barreras
            else if (barrierLeftActive && leftBarrier.getGlobalBounds().contains(ball.getPosition()))
            {
                ball.reverseX();
            }
            else if (barrierRightActive && rightBarrier.getGlobalBounds().contains(ball.getPosition()))
            {
                ball.reverseX();
            }

            // Comprobar colisiones con los bordes superior e inferior
            Vector2f pos = ball.getPosition();
            FloatRect ballBounds = ball.getSprite().getGlobalBounds();
            if (pos.y < 70 + ballBounds.height / 2 || pos.y > 550 - ballBounds.height / 2)
            {
                ball.reverseY();
            }

            // Comprobar si ha salido por los lados (gol)
            if (pos.x < 0)
            {
                // Gol para el jugador derecho
                if (doublePointsActive)
                {
                    rightScore += 2;
                }
                else if (lessPointsActive)
                {
                    rightScore += 0; // No dar puntos
                }
                else
                {
                    rightScore++;
                }
                updateScoreDisplay();
                goalScored = true;
                doublePointsActive = false;
                lessPointsActive = false;
                ball.reset();

                // No necesitamos comprobar victoria aquí, se hace en update()

                break; // Salir del bucle para evitar más procesamiento
            }
            else if (pos.x > 850)
            {
                // Gol para el jugador izquierdo
                if (doublePointsActive)
                {
                    leftScore += 2;
                }
                else if (lessPointsActive)
                {
                    leftScore += 0; // No dar puntos
                }
                else
                {
                    leftScore++;
                }
                updateScoreDisplay();
                goalScored = true;
                doublePointsActive = false;
                lessPointsActive = false;
                ball.reset();

                // No necesitamos comprobar victoria aquí, se hace en update()

                break; // Salir del bucle para evitar más procesamiento
            }
        }

        // Si se anotó un gol, reiniciar todas las pelotas
        if (goalScored)
        {
            balls.clear();
            Ball newBall(ballTexture);
            newBall.reset();         // Asegurarse de que la pelota tenga una velocidad inicial
            newBall.setActive(true); // Asegurar que esté visible
            balls.push_back(newBall);
            return;
        }

        // Eliminar pelotas inactivas
        balls.erase(
            remove_if(balls.begin(), balls.end(), [](const Ball &b)
                      { return !b.isActive(); }),
            balls.end());
    }

    void updatePaddles()
    {
        // Actualizar congelamiento
        if (freezeLeftActive && freezeTimerLeft.getElapsedTime().asSeconds() > 5.0f) // 5 segundos
        {
            freezeLeftActive = false;
        }
        if (freezeRightActive && freezeTimerRight.getElapsedTime().asSeconds() > 5.0f)
        {
            freezeRightActive = false;
        }
        // Actualizar invisibilidad
        if (invisibleLeftActive && invisibleTimerLeft.getElapsedTime().asSeconds() > 5.0f) // 5 segundos
        {
            invisibleLeftActive = false;
            leftPaddle.getSprite().setColor(Color(255, 255, 255, 255));
        }
        if (invisibleRightActive && invisibleTimerRight.getElapsedTime().asSeconds() > 5.0f)
        {
            invisibleRightActive = false;
            rightPaddle.getSprite().setColor(Color(255, 255, 255, 255));
        }
        // Actualizar efecto de paleta más grande
        if (biggerLeftActive && biggerTimerLeft.getElapsedTime().asSeconds() > 5.0f) // 5 segundos de duración
        {
            biggerLeftActive = false;
            leftPaddle.resetSize();
        }
        if (biggerRightActive && biggerTimerRight.getElapsedTime().asSeconds() > 5.0f)
        {
            biggerRightActive = false;
            rightPaddle.resetSize();
        }
        // Actualizar barreras
        if (barrierLeftActive && barrierTimerLeft.getElapsedTime().asSeconds() > 5.0f) // 5 segundos
        {
            barrierLeftActive = false;
        }
        if (barrierRightActive && barrierTimerRight.getElapsedTime().asSeconds() > 5.0f)
        {
            barrierRightActive = false;
        }

        // Controlar paleta izquierda (jugador 1 o IA)
        if (!leftPaddle.getIsAI() && !freezeLeftActive)
        {
            if (Keyboard::isKeyPressed(Keyboard::W))
            {
                leftPaddle.move(-leftPaddle.getSpeed());
            }
            if (Keyboard::isKeyPressed(Keyboard::S))
            {
                leftPaddle.move(leftPaddle.getSpeed());
            }
        }
        else if (leftPaddle.getIsAI())
        {
            leftPaddle.update(balls, true);
        }

        else
        {
            leftPaddle.update(balls, true);
        }

        // Controlar paleta derecha (jugador 2 o IA)
        if (!rightPaddle.getIsAI() && !freezeRightActive)
        {
            if (Keyboard::isKeyPressed(Keyboard::Up))
            {
                rightPaddle.move(-rightPaddle.getSpeed());
            }
            if (Keyboard::isKeyPressed(Keyboard::Down))
            {
                rightPaddle.move(rightPaddle.getSpeed());
            }
        }
        else if (rightPaddle.getIsAI())
        {
            rightPaddle.update(balls, false);
        }
    }

    void spawnPowerUp()
    {
        if (powerUps.size() >= 3)
            return; // Máximo 3 power-ups a la vez

        int typeIndex = rand() % 11; // Ahora son 9 tipos
        PowerUpType type = static_cast<PowerUpType>(typeIndex);

        // Validar que LESS_POINTS solo salga si ambos tienen al menos 1 punto
        if (type == LESS_POINTS && (leftScore < 1 || rightScore < 1))
        {
            // No generar el LESS_POINTS, cambia el power-up a uno normal
            type = static_cast<PowerUpType>(rand() % 10);
        }
        PowerUp newPowerUp(type, powerUpTextures[type]);
        powerUps.push_back(newPowerUp);
    }

    void updatePowerUps()
    {
        for (auto &powerUp : powerUps)
        {
            if (!powerUp.isActive())
                continue;

            powerUp.update();

            // Comprobar colisiones con las pelotas si no ha sido recogido
            if (!powerUp.isCollected())
            {
                for (auto &ball : balls)
                {
                    if (ball.isActive() && powerUp.getSprite().getGlobalBounds().contains(ball.getPosition()))
                    {
                        applyPowerUp(powerUp);
                        powerUp.collect();
                        break;
                    }
                }
            }
        }

        // Eliminar power-ups inactivos
        powerUps.erase(
            remove_if(powerUps.begin(), powerUps.end(),
                      [](const PowerUp &p)
                      { return !p.isActive(); }),
            powerUps.end());
    }

    void applyPowerUp(PowerUp &powerUp)
    {
        bool isLeftPaddle = false;
        if (!balls.empty() && balls[0].getVelocity().x > 0)
        {
            isLeftPaddle = true;
        }

        switch (powerUp.getType())
        {
        case BIGGER_PADDLE:
            // Determinar qué jugador recibe el power-up (el que golpeó la pelota)
            if (isLeftPaddle)
            {
                leftPaddle.setSize(1.5f); // Aumentar tamaño en un 50%
                biggerLeftActive = true;
                biggerTimerLeft.restart();
            }
            else
            {
                rightPaddle.setSize(1.5f);
                biggerRightActive = true;
                biggerTimerRight.restart();
            }
            break;
        case SMALLER_OPPONENT:
            if (isLeftPaddle)
            {
                // Si el jugador izquierdo recoge el power-up, la paleta derecha se hace más pequeña
                rightPaddle.setSize(0.5f);
                smallerRightActive = true;
                smallerTimerRight.restart();
            }
            else
            {
                // Si el jugador derecho recoge el power-up, la paleta izquierda se hace más pequeña
                leftPaddle.setSize(0.5f);
                smallerLeftActive = true;
                smallerTimerLeft.restart();
            }
            break;
        case SLOW_BALL:
            for (auto &ball : balls)
            {
                ball.slowDown(0.7f); // Reducir velocidad en un 30%
            }
            break;
        case DOUBLE_BALL:
            if (balls.size() < 2)
            {
                balls.push_back(Ball(ballTexture));
                balls.back().reset();
            }
            break;
        case BARRIER:
            // Crear una barrera para el jugador que golpeó la pelota
            if (isLeftPaddle)
            {
                barrierLeftActive = true;
                barrierTimerLeft.restart();
                leftBarrier.setSize(Vector2f(10, 100));
                leftBarrier.setPosition(100, 250);
                leftBarrier.setFillColor(Color(100, 100, 255, 150));
            }
            else
            {
                barrierRightActive = true;
                barrierTimerRight.restart();
                rightBarrier.setSize(Vector2f(10, 100));
                rightBarrier.setPosition(750, 250);
                rightBarrier.setFillColor(Color(255, 100, 100, 150));
            }
            break;
        case INVERT_CONTROLS:
            if (isLeftPaddle)
            {
                rightPaddle.setInvertedControls(true);
            }
            else
            {
                leftPaddle.setInvertedControls(true);
            }
            break;
        case FLASHING_BALL:
            // Hacer que todas las pelotas parpadeen durante 5 segundos
            for (auto &ball : balls)
            {
                ball.startFlashing(5.0f); // 5 segundos de parpadeo
            }
            break;
        case DOUBLE_POINTS:
            doublePointsActive = true;
            doublePointsTimer.restart();
            break;
        case LESS_POINTS:
            lessPointsActive = true;
            lessPointsTimer.restart();
            break;
        case FREEZE_OPPONENT:
            if (isLeftPaddle)
            {
                freezeRightActive = true;
                freezeTimerRight.restart();
            }
            else
            {
                freezeLeftActive = true;
                freezeTimerLeft.restart();
            }
            break;
        case INVISIBLE_OPPONENT:
            if (isLeftPaddle)
            {
                invisibleRightActive = true;
                invisibleTimerRight.restart();
            }
            else
            {
                invisibleLeftActive = true;
                invisibleTimerLeft.restart();
            }
            break;
        }
    }

    void resetPowerUpEffects()
    {
        leftPaddle.resetSize();
        rightPaddle.resetSize();
        rightPaddle.setInvertedControls(false);

        // Mantener solo una pelota
        while (balls.size() > 1)
        {
            balls.pop_back();
        }

        // Eliminar todos los power-ups
        powerUps.clear();
    }

    void render()
    {
        window.clear(Color(0, 0, 0));

        if (state == MENU)
        {
            menu->draw(window);
        }
        else
        {
            // Dibujar la barra de separación
            window.draw(headerBar);

            // Dibujar línea central
            RectangleShape centerLine(Vector2f(2, 480));
            centerLine.setPosition(425, 70);
            centerLine.setFillColor(Color(255, 255, 255, 100));
            window.draw(centerLine);

            // Dibujar pelotas
            for (const auto &ball : balls)
            {
                if (ball.isActive() && ball.isVisible()) // <-- usar ambos
                {
                    window.draw(ball.getSprite());
                }
            }

            // Dibujar paletas
            if (!invisibleLeftActive)
                window.draw(leftPaddle.getSprite());
            if (!invisibleRightActive)
                window.draw(rightPaddle.getSprite());

            // Dibujar barreras si están activas
            if (barrierLeftActive)
            {
                window.draw(leftBarrier);
            }
            
            if (barrierRightActive)
            {
                window.draw(rightBarrier);
            }

            // Dibujar power-ups
            for (const auto &powerUp : powerUps)
            {
                if (powerUp.isActive() && !powerUp.isCollected())
                {
                    window.draw(powerUp.getSprite());
                }
            }

            // Dibujar puntuación
            window.draw(scoreLeft);
            window.draw(scoreRight);

            // Dibujar temporizador
            window.draw(timer->getDisplay());

            // Si el juego está pausado, mostrar texto de pausa
            if (state == PAUSED)
            {
                window.draw(pauseText);
            }

            // Si el juego ha terminado, mostrar texto de fin de juego
            if (state == GAME_OVER)
            {
                window.draw(gameOverText);
            }
        }

        window.display();
    }

    void resetGame()
    {
        // Limpiar pelotas y power-ups
        balls.clear();
        powerUps.clear();

        // Crear una nueva pelota con velocidad inicial
        balls.push_back(Ball(ballTexture));

        // Asegurarse de que la pelota tenga una velocidad inicial
        balls[0].reset();

        // Reiniciar puntuaciones
        leftScore = 0;
        rightScore = 0;
        doublePointsActive = false;
        lessPointsActive = false;
        freezeLeftActive = false;
        freezeRightActive = false;
        invisibleLeftActive = false;
        invisibleRightActive = false;
        biggerLeftActive = false;
        biggerRightActive = false;
        barrierLeftActive = false;
        barrierRightActive = false;
        smallerLeftActive = false;
        smallerRightActive = false;
        doublePointsActive = false;
        lessPointsActive = false;
        updateScoreDisplay();

        // Reiniciar temporizador
        timer->reset();

        // Reiniciar paletas
        leftPaddle.resetSize();
        rightPaddle.resetSize();
        leftPaddle.setInvertedControls(false);
        rightPaddle.setInvertedControls(false);

        // Aplicar configuraciones del menú
        maxScore = menu->getMaxScore();
        powerUpsEnabled = menu->arePowerUpsEnabled();

        // Reiniciar temporizador de power-ups
        powerUpSpawnTimer.restart();
    }

    void updateScoreDisplay()
    {
        scoreLeft.setString(to_string(leftScore));
        scoreRight.setString(to_string(rightScore));
    }

    void handlePlayerInput()
    {
        // Controles para la paleta izquierda (si no es IA)
        if (!leftPaddle.getIsAI())
        {
            if (Keyboard::isKeyPressed(Keyboard::W))
            {
                leftPaddle.move(-leftPaddle.getSpeed()); // Move up
            }
            if (Keyboard::isKeyPressed(Keyboard::S))
            {
                leftPaddle.move(leftPaddle.getSpeed()); // Move down
            }
        }

        // Controles para la paleta derecha (si no es IA)
        if (!rightPaddle.getIsAI())
        {
            if (Keyboard::isKeyPressed(Keyboard::Up))
            {
                rightPaddle.move(-rightPaddle.getSpeed()); // Move up
            }
            if (Keyboard::isKeyPressed(Keyboard::Down))
            {
                rightPaddle.move(rightPaddle.getSpeed()); // Move down
            }
        }
    }

    void showOptionsMenu()
    {
        // Crear un menú de opciones simple
        bool optionsMenuOpen = true;
        int selectedOption = 0;
        vector<string> options = {
            "Dificultad IA 1: " + to_string(static_cast<int>(menu->getAILevel1())),
            "Dificultad IA 2: " + to_string(static_cast<int>(menu->getAILevel2())),
            "Duracion Partida: " + to_string(menu->getGameDuration()) + " min",
            "Puntuacion Maxima: " + to_string(menu->getMaxScore()),
            "Power-Ups: " + string(menu->arePowerUpsEnabled() ? "Activados" : "Desactivados"),
            "Volver"};

        while (optionsMenuOpen && window.isOpen())
        {
            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed)
                {
                    window.close();
                    return;
                }

                if (event.type == Event::KeyPressed)
                {
                    if (event.key.code == Keyboard::Up)
                    {
                        selectedOption = (selectedOption - 1 + options.size()) % options.size();
                    }
                    else if (event.key.code == Keyboard::Down)
                    {
                        selectedOption = (selectedOption + 1) % options.size();
                    }
                    else if (event.key.code == Keyboard::Left)
                    {
                        // Disminuir valor
                        switch (selectedOption)
                        {
                        case 0: // Nivel IA 1
                            if (menu->getAILevel1() > EASY)
                                menu->setAILevel1(static_cast<AILevel>(static_cast<int>(menu->getAILevel1()) - 1));
                            break;
                        case 1: // Nivel IA 2
                            if (menu->getAILevel2() > EASY)
                                menu->setAILevel2(static_cast<AILevel>(static_cast<int>(menu->getAILevel2()) - 1));
                            break;
                        case 2: // Duración del juego
                            if (menu->getGameDuration() > 1)
                                menu->setGameDuration(menu->getGameDuration() - 1);
                            break;
                        case 3: // Puntuación máxima
                            if (menu->getMaxScore() > 3)
                                menu->setMaxScore(menu->getMaxScore() - 2);
                            break;
                        case 4: // Power-Ups
                            menu->setPowerUpsEnabled(!menu->arePowerUpsEnabled());
                            break;
                        }
                    }
                    else if (event.key.code == Keyboard::Right)
                    {
                        // Aumentar valor
                        switch (selectedOption)
                        {
                        case 0: // Nivel IA 1
                            if (menu->getAILevel1() < IMPOSSIBLE)
                                menu->setAILevel1(static_cast<AILevel>(static_cast<int>(menu->getAILevel1()) + 1));
                            break;
                        case 1: // Nivel IA 2
                            if (menu->getAILevel2() < IMPOSSIBLE)
                                menu->setAILevel2(static_cast<AILevel>(static_cast<int>(menu->getAILevel2()) + 1));
                            break;
                        case 2: // Duración del juego
                            if (menu->getGameDuration() < 10)
                                menu->setGameDuration(menu->getGameDuration() + 1);
                            break;
                        case 3: // Puntuación máxima
                            if (menu->getMaxScore() < 21)
                                menu->setMaxScore(menu->getMaxScore() + 2);
                            break;
                        case 4: // Power-Ups
                            menu->setPowerUpsEnabled(!menu->arePowerUpsEnabled());
                            break;
                        }
                    }
                    else if (event.key.code == Keyboard::Return)
                    {
                        if (selectedOption == 5) // Volver
                        {
                            optionsMenuOpen = false;
                        }
                    }
                    else if (event.key.code == Keyboard::Escape)
                    {
                        optionsMenuOpen = false;
                    }

                    // Actualizar texto de opciones
                    options[0] = "Dificultad IA 1: " + to_string(static_cast<int>(menu->getAILevel1()));
                    options[1] = "Dificultad IA 2: " + to_string(static_cast<int>(menu->getAILevel2()));
                    options[2] = "Duracion Partida: " + to_string(menu->getGameDuration()) + " min";
                    options[3] = "Puntuacion Maxima: " + to_string(menu->getMaxScore());
                    options[4] = "Power-Ups: " + string(menu->arePowerUpsEnabled() ? "Activados" : "Desactivados");
                }
            }

            // Renderizar menú de opciones
            window.clear(Color(0, 0, 0));

            Text title("OPCIONES", font, 50);
            FloatRect titleBounds = title.getLocalBounds();
            title.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top);
            title.setPosition(850 / 2.0f, 50);

            window.draw(title);

            for (size_t i = 0; i < options.size(); i++)
            {
                Text optionText(options[i], font, 30);
                FloatRect optionBounds = optionText.getLocalBounds();
                optionText.setOrigin(optionBounds.left + optionBounds.width / 2.0f, optionBounds.top);
                optionText.setPosition(850 / 2.0f, 150 + i * 50);

                // Resaltar opción seleccionada
                if (i == selectedOption)
                    optionText.setFillColor(Color::Yellow);
                else
                    optionText.setFillColor(Color::White);

                window.draw(optionText);
            }

            window.display();
        }
    }

    void checkPowerUpCollisions()
    {
        for (auto &ball : balls)
        {
            if (!ball.isActive())
                continue;

            for (auto it = powerUps.begin(); it != powerUps.end();)
            {
                if (!it->isActive() || it->isCollected())
                {
                    it = powerUps.erase(it);
                    continue;
                }

                if (it->getSprite().getGlobalBounds().contains(ball.getPosition()))
                {
                    applyPowerUp(*it);
                    it->collect();
                    it = powerUps.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    // Método para mostrar el menú de dificultad de la IA
    void showAIDifficultyMenu()
    {
        // Crear un menú temporal para la selección de dificultad
        RenderWindow difficultyWindow(VideoMode(600, 300), "Seleccionar Dificultad");
        difficultyWindow.setFramerateLimit(60);

        // Opciones de dificultad
        vector<Text> options;
        vector<string> difficultyNames = {"FACIL", "MEDIA", "DIFICIL", "IMPOSIBLE"};
        vector<AILevel> difficultyLevels = {EASY, MEDIUM, HARD, IMPOSSIBLE};

        int selectedOption = 0;

        // Crear las opciones de texto
        for (int i = 0; i < 4; i++)
        {
            Text option(difficultyNames[i], font, 30);
            option.setPosition(150, 80 + i * 50);
            if (i == selectedOption)
            {
                option.setFillColor(Color::Yellow);
            }
            else
            {
                option.setFillColor(Color::White);
            }
            options.push_back(option);
        }

        // Título
        Text title("SELECCIONA DIFICULTAD", font, 24);
        title.setPosition(80, 30);

        // Loop principal del menú de dificultad
        while (difficultyWindow.isOpen())
        {
            Event event;
            while (difficultyWindow.pollEvent(event))
            {
                if (event.type == Event::Closed)
                {
                    difficultyWindow.close();
                }

                if (event.type == Event::KeyPressed)
                {
                    if (event.key.code == Keyboard::Up)
                    {
                        options[selectedOption].setFillColor(Color::White);
                        selectedOption = (selectedOption - 1 + 4) % 4;
                        options[selectedOption].setFillColor(Color::Yellow);
                    }
                    else if (event.key.code == Keyboard::Down)
                    {
                        options[selectedOption].setFillColor(Color::White);
                        selectedOption = (selectedOption + 1) % 4;
                        options[selectedOption].setFillColor(Color::Yellow);
                    }
                    else if (event.key.code == Keyboard::Return)
                    {
                        // Configurar el juego con la dificultad seleccionada
                        gameMode = PLAYER_VS_AI;
                        leftPaddle.setIsAI(false);
                        rightPaddle.setIsAI(true);
                        rightPaddle.setAILevel(difficultyLevels[selectedOption]);
                        menu->setAILevel1(difficultyLevels[selectedOption]); // Guardar la selección
                        resetGame();
                        state = PLAYING;
                        difficultyWindow.close();
                    }
                    else if (event.key.code == Keyboard::Escape)
                    {
                        difficultyWindow.close();
                    }
                }
            }

            // Dibujar
            difficultyWindow.clear(Color(0, 0, 0));
            difficultyWindow.draw(title);
            for (const auto &option : options)
            {
                difficultyWindow.draw(option);
            }
            difficultyWindow.display();
        }
    }

    void applyPowerUp(const PowerUp &powerUp)
    {
        bool isLeftPaddle = false;
        if (!balls.empty() && balls[0].getVelocity().x > 0)
        {
            isLeftPaddle = true;
        }

        switch (powerUp.getType())
        {
        case BIGGER_PADDLE:
            // Determinar qué jugador recibe el power-up (el que golpeó la pelota)
            if (isLeftPaddle)
            {
                leftPaddle.setSize(1.5f); // Aumentar tamaño en 50%
                biggerLeftActive = true;
                biggerTimerLeft.restart();
            }
            else
            {
                rightPaddle.setSize(1.5f);
                biggerRightActive = true;
                biggerTimerRight.restart();
            }
            break;

        case SMALLER_OPPONENT:
            // Reducir el tamaño del oponente
            if (isLeftPaddle)
            {
                // Si el jugador izquierdo recoge el power-up, la paleta derecha se hace más pequeña
                rightPaddle.setSize(0.5f);
                smallerRightActive = true;
                smallerTimerRight.restart();
            }
            else
            {
                // Si el jugador derecho recoge el power-up, la paleta izquierda se hace más pequeña
                leftPaddle.setSize(0.5f);
                smallerLeftActive = true;
                smallerTimerLeft.restart();
            }
            break;

        case SLOW_BALL:
            // Reducir velocidad de todas las pelotas
            for (auto &ball : balls)
            {
                ball.slowDown(0.7f); // Reducir velocidad en 30%
            }
            break;

        case DOUBLE_BALL:
            // Añadir una segunda pelota
            if (balls.size() < 2)
            { // Máximo 2 pelotas
                balls.push_back(Ball(ballTexture));
            }
            break;

        case BARRIER:
            // Crear una barrera para el jugador que golpeó la pelota
            if (isLeftPaddle)
            {
                barrierLeftActive = true;
                barrierTimerLeft.restart();
                leftBarrier.setSize(Vector2f(10, 100));
                leftBarrier.setPosition(100, 250);
                leftBarrier.setFillColor(Color(100, 100, 255, 150));
            }
            else
            {
                barrierRightActive = true;
                barrierTimerRight.restart();
                rightBarrier.setSize(Vector2f(10, 100));
                rightBarrier.setPosition(750, 250);
                rightBarrier.setFillColor(Color(255, 100, 100, 150));
            }
            break;

        case INVERT_CONTROLS:
            // Invertir controles del oponente
            if (isLeftPaddle)
            {
                rightPaddle.setInvertedControls(true);
            }
            else
            {
                leftPaddle.setInvertedControls(true);
            }
            break;

        case FLASHING_BALL:
            // Hacer que todas las pelotas parpadeen durante 5 segundos
            for (auto &ball : balls)
            {
                ball.startFlashing(5.0f);
            }
            break;

        case DOUBLE_POINTS:
            doublePointsActive = true;
            doublePointsTimer.restart();
            break;

        case LESS_POINTS:
            lessPointsActive = true;
            lessPointsTimer.restart();
            break;

        case FREEZE_OPPONENT:
            if (isLeftPaddle)
            {
                freezeRightActive = true;
                freezeTimerRight.restart();
            }
            else
            {
                freezeLeftActive = true;
                freezeTimerLeft.restart();
            }
            break;

        case INVISIBLE_OPPONENT:
            if (isLeftPaddle)
            {
                invisibleRightActive = true;
                invisibleTimerRight.restart();
            }
            else
            {
                invisibleLeftActive = true;
                invisibleTimerLeft.restart();
            }
            break;
        }
    }

    void handleCollisions()
    {
        // Colisión con las barreras
        if (barrierLeftActive)
        {
            FloatRect barrierBounds = leftBarrier.getGlobalBounds();
            for (auto &ball : balls)
            {
                if (!ball.isActive())
                    continue;
                
                FloatRect ballBounds = ball.getSprite().getGlobalBounds();
                if (ballBounds.intersects(barrierBounds))
                {
                    // Invertir la dirección horizontal de la pelota
                    ball.reverseX();
                    // Mover la pelota fuera de la barrera para evitar colisiones múltiples
                    Vector2f ballPos = ball.getPosition();
                    if (ball.getVelocity().x < 0)
                    {
                        ball.getSprite().setPosition(barrierBounds.left + barrierBounds.width + ballBounds.width/2, ballPos.y);
                    }
                    else
                    {
                        ball.getSprite().setPosition(barrierBounds.left - ballBounds.width/2, ballPos.y);
                    }
                }
            }
        }
        
        if (barrierRightActive)
        {
            FloatRect barrierBounds = rightBarrier.getGlobalBounds();
            for (auto &ball : balls)
            {
                if (!ball.isActive())
                    continue;
                    
                FloatRect ballBounds = ball.getSprite().getGlobalBounds();
                if (ballBounds.intersects(barrierBounds))
                {
                    // Invertir la dirección horizontal de la pelota
                    ball.reverseX();
                    // Mover la pelota fuera de la barrera para evitar colisiones múltiples
                    Vector2f ballPos = ball.getPosition();
                    if (ball.getVelocity().x < 0)
                    {
                        ball.getSprite().setPosition(barrierBounds.left + barrierBounds.width + ballBounds.width/2, ballPos.y);
                    }
                    else
                    {
                        ball.getSprite().setPosition(barrierBounds.left - ballBounds.width/2, ballPos.y);
                    }
                }
            }
        }
    }
};

int main()
{
    Game game;
    game.run();
    return 0;
}
