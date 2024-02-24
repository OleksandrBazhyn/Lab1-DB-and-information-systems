#include <fstream>
#include <iostream>
#include <string>

struct Books
{
    uint32_t key = 0;
    uint32_t author_code = 0;
    uint32_t genre_code = 0;
    uint32_t isbn = 0;

    char name[25] = {};
};

struct Genre
{
    uint32_t key = 0;
    char name[25] = {};
};

struct Authors
{
    uint32_t key = 0;
    char name[25] = {};
};

struct IndexGenre {
    uint32_t key = 0;
    long position; // В теорії key * sizeof(Genre) + sizeof(uint32_t)
};

static Genre& GetM(uint32_t GenreKey) { // Використовує індексну таблицю. Спочатку шукає ключ, потім бере адресу в fl файлі і повертає об'єкт звідти
    std::ifstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }

    // Зчитати індекс з файлу
    IndexGenre currentIndex;
    bool found = false;

    indexGenresFile.seekg(0, std::ios::end);

    // Отримати останньої позиції (позначка поточної позиції в кінці файлу)
    std::streampos endFilePos = indexGenresFile.tellg();

    indexGenresFile.seekg(0, std::ios::beg);

    while (true) {
        if (indexGenresFile.tellg() == 0) break;
        indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre));
        if (currentIndex.key == GenreKey) {
            found = true;
            break;
        }
    }

    indexGenresFile.close();

    if (found) {
        std::ifstream genresFile("genresFile.fl", std::ios::binary | std::ios::in);
        if (!genresFile.is_open()) {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }

        // Перейти до позиції запису в основному файлі
        genresFile.seekg(currentIndex.position, std::ios::beg);

        Genre desiredGenre;
        genresFile.read(reinterpret_cast<char*>(&desiredGenre), sizeof(Genre));

        genresFile.close();

        return desiredGenre;
    }
    else {
        std::cerr << "Genre not found" << std::endl;
    }
}

void AddGenre(Genre& newGenre) {
    // Додаємо новий запис в основний файл
    std::fstream genresFile("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFile.is_open()) {
        std::cerr << "Unable to open file genresFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file genresFile.fl");
    }

    // Дізнатися кількість записів в файлі. Ця інформація на початку файлу (header)
    genresFile.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    genresFile.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    genresFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію

    recordsCount++;
    newGenre.key = recordsCount;

    std::streampos newPosition = genresFile.tellp();  // Знайти позицію нового запису

    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Додали новий запис та збільшили recordsCount у файлі на одиницю

    genresFile.flush();
    genresFile.close();

    // Оновлюємо індексну таблицю
    std::fstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    // indexGenresFile.seekg(0, std::ios::end);     з std::ios::ate прапором і без цього позиція буде в кінці

    // Отримати розмір файлу (позначка поточної позиції в кінці файлу)
    std::streampos endFilePos = indexGenresFile.tellg();

    // Визначити кількість записів у файлі
    uint32_t indexRecordsCount = endFilePos / sizeof(IndexGenre);

    // Перейти до останнього запису
    indexGenresFile.seekg(-static_cast<std::streamoff>(sizeof(IndexGenre)), std::ios::cur);

    // Прочитати останній запис
    IndexGenre lastIndex;
    indexGenresFile.read(reinterpret_cast<char*>(&lastIndex), sizeof(IndexGenre));

    // Отримати останній ключ
    uint32_t lastKey = indexRecordsCount > 0 ? lastIndex.key : 0;

    // Збільшуємо ключ для нового запису
    uint32_t newKey = lastKey + 1;

    // Додаємо новий запис в індекс
    IndexGenre newIndex;
    newIndex.key = newKey;
    newIndex.position = newPosition;

    indexGenresFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію
    indexGenresFile.write(reinterpret_cast<const char*>(&newIndex), sizeof(IndexGenre));
}

bool Read(Genre& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&record), sizeof(Genre));

    return !file.fail();
}

bool Write(const Genre& record, std::fstream& file, const std::streampos& pos)
{
    if (!file)
        return false;

    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&record), sizeof(Genre));
    file.flush();

    return !file.fail();
}

int main()
{
    Genre newGenre = {0, "new Genre"};
    AddGenre(newGenre);

    // Випробуємо GetM
    uint32_t genreKeyToFind = 1; // Припустимо, що це ключ жанру, який ви шукаєте
    GetM(genreKeyToFind);


    return 0;
}

