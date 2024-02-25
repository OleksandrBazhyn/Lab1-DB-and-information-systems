#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>

struct Book
{
    uint32_t key = 0;
    uint32_t genre_code = 0;
    uint32_t isbn = 0;

    char name[25] = {};

    Book() : key(0), genre_code(0), isbn(0)
    {
        name[0] = '\0';
    }

    Book(size_t k, uint32_t gc, uint32_t i, const char* n) : key(k), genre_code(gc), isbn(i)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct Genre
{
    uint32_t key = 0;
    char name[25] = {};

    Genre() : key(0)
    {
        name[0] = '\0';
    }

    Genre(uint32_t k, const char* n) : key(k)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct IndexGenre {
    uint32_t key = 0;
    long position; // В теорії key * sizeof(Genre) + sizeof(uint32_t)
};

std::vector<Genre> GetM(uint32_t GenreKey) { // Використовує індексну таблицю. Спочатку шукає ключ, потім бере адресу в fl файлі і повертає об'єкт звідти
    std::ifstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in);
    
    if (!indexGenresFile.is_open())
    {
        std::ofstream createIndexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::trunc);
        if (!indexGenresFile.is_open())
        {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
        createIndexGenresFile.close();
        indexGenresFile = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in);
    }

    // Зчитати індекс з файлу
    IndexGenre currentIndex;
    std::vector<IndexGenre> neededIndexesGenre = {};
    bool foundAll = false;

    indexGenresFile.seekg(0, std::ios::beg);

    while (true) {
        indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre));
        if (indexGenresFile.eof())
        {
            break;
            
        }
        if (currentIndex.key == GenreKey)
        {
            neededIndexesGenre.push_back(currentIndex);
            foundAll = true;
        }
    }

    indexGenresFile.close();

    if (foundAll)
    {
        std::ifstream genresFile("genresFile.fl", std::ios::binary | std::ios::in);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }

        std::vector<Genre> neededGenres(neededIndexesGenre.size());

        for (size_t i = 0; i < neededIndexesGenre.size(); i++)
        {
            // Перейти до позиції запису в основному файлі
            genresFile.seekg(neededIndexesGenre[i].position, std::ios::beg);

            genresFile.read(reinterpret_cast<char*>(&neededGenres[i]), sizeof(Genre));

            genresFile.close();
        }
        return neededGenres;
    }
    else
    {
        std::cerr << "Genre not found" << std::endl;
    }
}

std::vector<Book> GetS(uint32_t genreCode) {
    std::ifstream booksFile("booksFile.fl", std::ios::binary | std::ios::in);
    if (!booksFile.is_open()) {
        std::cerr << "Unable to open file booksFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file booksFile.fl");
    }

    booksFile.seekg(sizeof(uint32_t), std::ios::beg); // Переступив через header, що містить інформацію про кількість записів

    Book currentBook;
    std::vector<Book> neededBooks;
    bool foundAll = false;

    while (true) {
        booksFile.read(reinterpret_cast<char*>(&currentBook), sizeof(Book));
        if (booksFile.eof()) {
            break;
        }
        if (currentBook.genre_code == genreCode) {
            neededBooks.push_back(currentBook);
            foundAll = true;
        }
    }

    booksFile.close();

    if (foundAll)
    {
        return neededBooks;
    }
    else
    {
        std::cerr << "Book not found" << std::endl;
        booksFile.close();
    }
}

void AddGenre(Genre& newGenre) {
    std::ifstream genresFileForRead("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFileForRead.is_open()) {
        genresFileForRead = std::ifstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFileForRead.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    // Дізнатися кількість записів в файлі. Ця інформація на початку файлу (header)
    genresFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    genresFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    genresFileForRead.close();

    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFile.is_open()) {
        genresFile = std::ofstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    if (recordsCount == 0)
    {
        genresFile.seekp(0, std::ios::beg);
        uint32_t i = 1;
        genresFile.write(reinterpret_cast<const char*>(&i), sizeof(uint32_t));
    }

    genresFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію

    recordsCount++;
    newGenre.key = recordsCount;

    std::streampos newPosition = genresFile.tellp();  // Знайти позицію нового запису

    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Додали новий запис зі збільшеним recordsCount на одиницю

    genresFile.flush();
    genresFile.close();

    // Оновлюємо індексну таблицю
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFileForRead.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFileForRead.seekg(0, std::ios::end);

    // Отримати розмір файлу (позначка поточної позиції в кінці файлу)
    std::streampos endFilePos = indexGenresFileForRead.tellg();

    // Визначити кількість записів у файлі
    uint32_t indexRecordsCount = endFilePos / sizeof(IndexGenre);

    // Перейти до останнього запису
    indexGenresFileForRead.seekg(-static_cast<std::streamoff>(sizeof(IndexGenre)), std::ios::cur);

    // Прочитати останній запис
    IndexGenre lastIndex;
    indexGenresFileForRead.read(reinterpret_cast<char*>(&lastIndex), sizeof(IndexGenre));

    indexGenresFileForRead.close();

    // Отримати останній ключ
    uint32_t lastKey = indexRecordsCount > 0 ? lastIndex.key : 0;

    // Збільшуємо ключ для нового запису
    uint32_t newKey = lastKey + 1;

    // Додаємо новий запис в індекс
    IndexGenre newIndex;
    newIndex.key = newKey;
    newIndex.position = newPosition;

    std::ofstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }

    indexGenresFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію
    indexGenresFile.write(reinterpret_cast<const char*>(&newIndex), sizeof(IndexGenre));

    indexGenresFile.flush();
    indexGenresFile.close();
}

void AddBook(Book& newBook) {
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open()) {
        booksFileForRead = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFileForRead.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    // Дізнатися кількість записів в файлі. Ця інформація на початку файлу (header)
    booksFileForRead.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    booksFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    booksFileForRead.close();

    std::ofstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open()) {
        booksFile = std::ofstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    if (recordsCount == 0)
    {
        booksFile.seekp(0, std::ios::beg);
        uint32_t i = 1;
        booksFile.write(reinterpret_cast<const char*>(&i), sizeof(uint32_t));
    }

    booksFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію

    recordsCount++;
    newBook.key = recordsCount;

    booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

    booksFile.seekp(0, std::ios::beg);
    booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Додали новий запис зі збільшеним recordsCount на одиницю

    booksFile.flush();
    booksFile.close();
}

int main()
{
    /*
    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Genre" + std::to_string(i);
        Genre f(i, name.c_str());
        AddGenre(f);
    }
    */
    /*
    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Book" + std::to_string(i);
        uint32_t gc = rand() % (50 - 1 + 1) + 1;
        Book f(i, gc, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, name.c_str());
        AddBook(f);
    }
    */
    /*
    Book f(0, 1, 22222222, "testBook");
    AddBook(f); // Додано тестову книгу з кодом жанру 1
    */

    // Випробуємо GetM
    std::vector<Genre> resGetM = GetM(1);

    for (size_t i = 0; i < resGetM.size(); i++)
    {
        std::cerr << resGetM[i].key << " " << resGetM[i].name << std::endl;
    }

    std::vector<Book> resGetS = GetS(1);

    for (size_t i = 0; i < resGetS.size(); i++)
    {
        std::cerr << "Key: " << resGetS[i].key << " name: " << resGetS[i].name << " GenreCode: " << resGetS[i].genre_code << " ISBN: " << resGetS[i].isbn << std::endl;
    }

    return 0;
}

