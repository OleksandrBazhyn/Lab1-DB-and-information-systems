#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <map>

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

    Book(uint32_t gc, uint32_t i, const char* n) : key(0), genre_code(gc), isbn(i)
    {
        strncpy_s(name, n, _TRUNCATE);
    }

    Book(size_t k, uint32_t gc, uint32_t i, const char* n) : key(k), genre_code(gc), isbn(i)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct GarbageBook
{
    bool IsFree = false;
    long position;
};

struct Genre
{
    uint32_t key = 0;
    char name[25] = {};

    Genre() : key(0)
    {
        name[0] = '\0';
    }

    Genre(const char* n) : key(0)
    {
        strncpy_s(name, n, _TRUNCATE);
    }

    Genre(uint32_t k, const char* n) : key(k)
    {
        strncpy_s(name, n, _TRUNCATE);
    }
};

struct IndexGenre 
{
    uint32_t key = 0;
    long position; // В теорії key * sizeof(Genre) + sizeof(uint32_t)
};

// Використовує індексну таблицю. Спочатку шукає ключ, потім бере адресу в fl файлі і повертає об'єкт звідти
static Genre GetM(uint32_t GenreKey)
{
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
    bool notFound = true;

    indexGenresFile.seekg(0, std::ios::beg);

    while (true)
    {
        indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre));
        if (indexGenresFile.eof())
        {
            std::cerr << "The Genre has been does not found." << std::endl;
            break;
        }
        if (currentIndex.key == GenreKey)
        {
            notFound = false;
            break;
        }        
    }
    if (notFound)
    {
        return { 0, "\0" };
    }

    indexGenresFile.close();

    std::ifstream genresFile("genresFile.fl", std::ios::binary | std::ios::in);
    if (!genresFile.is_open())
    {
        std::cerr << "Unable to open file genresFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file genresFile.fl");
    }

    // Перейти до позиції запису в основному файлі
    genresFile.seekg(currentIndex.position, std::ios::beg);
    Genre res;
    genresFile.read(reinterpret_cast<char*>(&res), sizeof(Genre));

    genresFile.close();

    return res;
}

std::vector<Book> GetS(uint32_t genreCode) {
    std::ifstream booksFile("booksFile.fl", std::ios::binary | std::ios::in);
    if (!booksFile.is_open())
    {
        booksFile = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    booksFile.seekg(sizeof(uint32_t), std::ios::beg); // Переступив через header, що містить інформацію про кількість записів

    Book currentBook;
    std::vector<Book> neededBooks;
    bool foundAll = false;

    while (true)
    {
        booksFile.read(reinterpret_cast<char*>(&currentBook), sizeof(Book));
        if (booksFile.eof())
        {
            break;
        }
        if (currentBook.genre_code == genreCode)
        {
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
        std::cerr << "Books have been not found" << std::endl;
        booksFile.close();
    }
}

// Видалення відбувається шляхом записування на обраний запис пустої струкутури (все по нулях) та в занотовувані цієї позиції в booksGarbageFile.gb спеціальною структурою. При додаванні елементів відповідний метод пукатиме серед booksGarbageFile.gb інформацію де може записати новий запис
static void DelS(uint32_t recordNumber)
{
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open())
    {
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

    // Якщо користувач хоче видалити запис за номером більшим ніж кількість записів
    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records." << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    std::ofstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open())
    {
        booksFile = std::ofstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }
    booksFile.seekp(0, std::ios::beg);
    recordsCount--;
    booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Зменшив показник кількості записів в booksFile.fl

    Book emptyBook;
    std::streampos delRecordPos = (recordNumber - 1) * sizeof(Book) + sizeof(uint32_t);

    booksFile.seekp(delRecordPos, std::ios::beg);
    booksFile.write(reinterpret_cast<const char*>(&emptyBook), sizeof(Book));

    booksFile.flush();
    booksFile.close();

    // Занотовую в якій позиції є видалена (зайнята порожньою структурою) пам'ять
    std::ofstream booksGarbageFile("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksGarbageFile.is_open())
    {
        booksGarbageFile = std::ofstream("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksGarbageFile.is_open())
        {
            std::cerr << "Unable to open file booksGarbageFile.gb" << std::endl;
            throw std::runtime_error("Unable to open file booksGarbageFile.gb");
        }
    }
    booksGarbageFile.seekp(0, std::ios::end);

    GarbageBook garbagebook = { true, delRecordPos };
    booksGarbageFile.write(reinterpret_cast<const char*>(&garbagebook), sizeof(GarbageBook));

    booksGarbageFile.flush();
    booksGarbageFile.close();
}

// Знаходить адресу(позицію) запису в основному файлі через індексну таблицю, видаляю запис з основної таблиці, підзаписи в іншому файлі та перезаписую індексну таблицю (прибираю останій елемент). Для економії місця(збору сміття), треба записати останій елемент на місце попереднього змінивши йому ключ в genresFiles.fl та в індексній таблиці видалити останній елемент, бо та позиція і ключ вже не існують в основному файлові
static void DelM(uint32_t recordNumber)
{
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

    // Записую в оперативну пам'ять останій елемент
    genresFileForRead.seekg(static_cast<std::streamsize>(sizeof(Genre) * (recordsCount - 1) + sizeof(uint32_t)), std::ios::beg);
    Genre lastGenre;
    genresFileForRead.read(reinterpret_cast<char*>(&lastGenre), sizeof(Genre));

    genresFileForRead.close();

    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records" << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    // Спроєктуємо індексну таблицю в оперативну пам'ять. В std::vector<IndexGenre> IndexGenreTable
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out);

    if (!indexGenresFileForRead.is_open()) {
        indexGenresFileForRead = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!indexGenresFileForRead.is_open())
        {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
    }
    indexGenresFileForRead.seekg(0, std::ios::beg);

    IndexGenre tmp;
    std::vector<IndexGenre> IndexGenreTable;

    while (indexGenresFileForRead.read(reinterpret_cast<char*>(&tmp), sizeof(IndexGenre))) {
        IndexGenreTable.push_back(tmp);
    }

    indexGenresFileForRead.close();
    
    // Видаляю master запис
    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::out | std::ios::in);
    if (!genresFile.is_open()) {
        genresFile = std::ofstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }
    // Записую на місце видаленого запису останній елемент (у мене є його клон в оперативній пам'яті), змінивши його ключ. Останній елемент видаляю
    genresFile.seekp(IndexGenreTable[recordNumber - 1].position, std::ios::beg);
    lastGenre.key = recordNumber;
    genresFile.write(reinterpret_cast<const char*>(&lastGenre), sizeof(Genre));

    genresFile.seekp(IndexGenreTable.back().position, std::ios::beg);
    Genre genreTmp;
    genresFile.write(reinterpret_cast<const char*>(&genreTmp), sizeof(Genre));

    // Змінюю лічильник
    genresFile.seekp(0, std::ios::beg);
    recordsCount--;
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t));

    genresFile.flush();
    genresFile.close();


    // Знаходжу підзаписи (для DelS мені треба їх ключі)
    std::ifstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open()) {
        booksFile = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }
    booksFile.seekg(sizeof(uint32_t), std::ios::beg);

    std::vector<uint32_t> booksKeys;
    Book bookTmp;

    while (true)
    {
        booksFile.read(reinterpret_cast<char*>(&bookTmp), sizeof(Book));
        if (booksFile.eof())
        {
            break;
        }
        if (bookTmp.genre_code == recordNumber)
        {
            booksKeys.push_back(bookTmp.key);
        }
    }

    booksFile.close();

    // Видалити підзаписи
    for (size_t i = 0; i < booksKeys.size(); i++)
    {
        DelS(booksKeys[i]);
    }

    // Записати відображення IndexGenreTable в індексну таблицю. Прибираю останній запис
    std::ofstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc); // Переписую файл начисто

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFile.seekp(0, std::ios::beg);

    IndexGenreTable.pop_back();

    for (size_t i = 0; i < IndexGenreTable.size(); i++)
    {
        indexGenresFile.write(reinterpret_cast<const char*>(&(IndexGenreTable[i])), sizeof(IndexGenre));
    }

    indexGenresFile.flush();
    indexGenresFile.close();
}

static void InsertM(Genre& newGenre) {
    // Оновлюємо індексну таблицю
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out);

    if (!indexGenresFileForRead.is_open()) {
        indexGenresFileForRead = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!indexGenresFileForRead.is_open())
        {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
    }
    indexGenresFileForRead.seekg(0, std::ios::beg);

    // Спроєктуємо індексну таблицю в оперативну пам'ять. В std::vector<IndexGenre> IndexGenreTable
    IndexGenre tmp;
    std::vector<IndexGenre> IndexGenreTable;

    while (indexGenresFileForRead.read(reinterpret_cast<char*>(&tmp), sizeof(IndexGenre))) {
        IndexGenreTable.push_back(tmp);
    }

    indexGenresFileForRead.close();

    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFile.is_open()) {
        genresFile = std::ofstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }

    if (IndexGenreTable.size() == 0)
    {
        genresFile.seekp(0, std::ios::beg);
        uint32_t i = 1;
        genresFile.write(reinterpret_cast<const char*>(&i), sizeof(uint32_t));
        IndexGenre newIndexGenre = { IndexGenreTable.size() + 1, sizeof(uint32_t) };
        IndexGenreTable.push_back(newIndexGenre);
    }
    else
    {
        IndexGenre newIndexGenre = { IndexGenreTable.size() + 1, IndexGenreTable.back().position + sizeof(Genre) };
        IndexGenreTable.push_back(newIndexGenre);
    }    
    
    newGenre.key = IndexGenreTable.size();
    
    genresFile.seekp(static_cast<std::streamsize>(IndexGenreTable.back().position), std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&newGenre.key), sizeof(uint32_t)); // Додали новий запис зі збільшеним лічильником на одиницю

    genresFile.flush();
    genresFile.close();

    // Записати відображення IndexGenreTable в індексну таблицю. Прибираю останній запис
    std::ofstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc); // Переписую файл начисто

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFile.seekp(0, std::ios::beg);

    for (size_t i = 0; i < IndexGenreTable.size(); i++)
    {
        indexGenresFile.write(reinterpret_cast<const char*>(&(IndexGenreTable[i])), sizeof(IndexGenre));
    }

    indexGenresFile.flush();
    indexGenresFile.close();
}

static void InsertS(Book& newBook) {
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

    std::ifstream booksGarbageFile("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksGarbageFile.is_open()) {
        booksGarbageFile = std::ifstream("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksGarbageFile.is_open())
        {
            std::cerr << "Unable to open file booksGarbageFile.gb" << std::endl;
            throw std::runtime_error("Unable to open file booksGarbageFile.gb");
        }
    }

    // Дізнатися за допомогою booksGarbageFile.gb де є вільні для запису бінарні ділянки в файлі booksFile.fl аби записати туди новий запис
    booksGarbageFile.seekg(0, std::ios::beg);
    GarbageBook garbageBook;
    bool found = false;

    booksGarbageFile.read(reinterpret_cast<char*>(&garbageBook), sizeof(GarbageBook));
    if (garbageBook.IsFree == true)
    {
        found = true;
    }

    booksGarbageFile.seekg(0, std::ios::beg);
    GarbageBook tmp;
    std::vector<GarbageBook> garbageBookList;
    while (booksGarbageFile.read(reinterpret_cast<char*>(&tmp), sizeof(GarbageBook))) {
        garbageBookList.push_back(tmp);
    }

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

    if (found)
    {
        booksFile.seekp(garbageBook.position, std::ios::beg);
        newBook.key = (garbageBook.position - 4) / sizeof(Book) + 1;
        booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

        recordsCount++;
        booksFile.seekp(0, std::ios::beg);
        booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Додали новий запис зі збільшеним recordsCount на одиницю

        booksFile.flush();
        booksFile.close();

        // Перезаписую смітник видаливши запис про використану ділянку (першу)
        std::ofstream booksGarbageFile("booksGarbageFile.gb", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

        if (!booksGarbageFile.is_open()) {
            if (!booksGarbageFile.is_open())
            {
                std::cerr << "Unable to open file booksGarbageFile.gb" << std::endl;
                throw std::runtime_error("Unable to open file booksGarbageFile.gb");
            }
        }

        garbageBookList.erase(garbageBookList.begin());
        booksGarbageFile.seekp(0, std::ios::beg);
        for (size_t i = 0; i < garbageBookList.size(); i++)
        {
            booksGarbageFile.write(reinterpret_cast<const char*>(&(garbageBookList[i])), sizeof(GarbageBook));
        }
        booksGarbageFile.flush();
        booksGarbageFile.close();
    }
    else
    {
        booksFile.seekp(0, std::ios::end);  // Перейти в кінцеву позицію
        recordsCount++;
        newBook.key = recordsCount;

        booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

        booksFile.seekp(0, std::ios::beg);
        booksFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // Додали новий запис зі збільшеним recordsCount на одиницю

        booksFile.flush();
        booksFile.close();
    }
}


static void UpdateS(uint32_t recordNumber, Book& newBook)
{
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open())
    {
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

    // Якщо користувач хоче оновити запис за номером більшим ніж кількість записів
    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records." << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    // Оновлюю запис
    std::ofstream booksFile("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFile.is_open())
    {
        booksFile = std::ofstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFile.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    std::streampos recordPos = (recordNumber - 1) * sizeof(Book) + sizeof(uint32_t);
    newBook.key = recordNumber;

    booksFile.seekp(recordPos, std::ios::beg);
    booksFile.write(reinterpret_cast<const char*>(&newBook), sizeof(Book));

    booksFile.flush();
    booksFile.close();
}

static void UpdateM(uint32_t recordNumber, Genre& newGenre)
{
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

    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records" << std::endl;
        return;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return;
    }

    // Знаходимо адресу потрібного запису по індекній таблиці
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out);

    if (!indexGenresFileForRead.is_open()) {
        indexGenresFileForRead = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!indexGenresFileForRead.is_open())
        {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
    }
    indexGenresFileForRead.seekg(0, std::ios::beg);

    IndexGenre neededGenreIndex;
    while (true)
    {
        indexGenresFileForRead.read(reinterpret_cast<char*>(&neededGenreIndex), sizeof(IndexGenre));
        if (indexGenresFileForRead.eof())
        {
            std::cerr << "The Genre has been does not found." << std::endl;
            break;
        }
        if (neededGenreIndex.key == recordNumber)
        {
            break;
        }
    }

    indexGenresFileForRead.close();

    // Оновлюю потрібний запис
    std::ofstream genresFile("genresFile.fl", std::ios::binary | std::ios::out | std::ios::in);
    if (!genresFile.is_open()) {
        genresFile = std::ofstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFile.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }
    // Записую на місце потрібного запису новий запис, змінивши того ключ
    genresFile.seekp(neededGenreIndex.position, std::ios::beg);
    newGenre.key = recordNumber;
    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.flush();
    genresFile.close();
}

// Підраховує кількість записів загалом та кількість підзаписів для обраного запису
static std::map<std::string, int> CalM(uint32_t recordNumber)
{
    std::map<std::string, int> res
    {
        {"Count of records", 0},
        {"Count of subrecords", 0}
    };

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

    res["Count of records"] = recordsCount;

    genresFileForRead.close();

    if (recordNumber > recordsCount)
    {
        std::cerr << "The record number exceeds the number of records." << std::endl;
        return res;
    }
    if (recordNumber == 0)
    {
        std::cerr << "We do not have 0th record." << std::endl;
        return res;
    }

    std::vector<Book> subrecords = GetS(recordNumber);
    res["Count of subrecords"] = subrecords.size();

    return res;
}

// Підраховує кількість записів в slave файлі
static int CalS(uint32_t recordNumber)
{
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open())
    {
        booksFileForRead = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFileForRead.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    booksFileForRead.seekg(0, std::ios::beg);
    uint32_t res = NULL;
    booksFileForRead.read(reinterpret_cast<char*>(&res), sizeof(uint32_t));

    booksFileForRead.close();

    return res;
}

// Відображає записи з master-file та підзаписи
static void UtM()
{
    std::ifstream genresFileForRead("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFileForRead.is_open())
    {
        genresFileForRead = std::ifstream("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!genresFileForRead.is_open())
        {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }
    }
    genresFileForRead.seekg(0, std::ios::beg);
    
    uint32_t recordsCount = 0;
    genresFileForRead.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    Genre tmp;

    for (size_t i = 1; i < (recordsCount + 1); i++)
    {
        genresFileForRead.read(reinterpret_cast<char*>(&tmp), sizeof(Genre));
        if (genresFileForRead.eof())
        {
            break;
        }
        if (tmp.key != 0)
        {
            std::cout << "ID\tName" << std::endl;

            std::cout << tmp.key << "\t"
                << tmp.name << std::endl;

            std::vector<Book> subrecords = GetS(i);

            if (!subrecords.empty())
            {
                std::cout << "subrecords: " << std::endl;
                std::cout << "\t\tBook ID\t\tName\t\t\tGenre Code\t\tISBN" << std::endl;
                for (size_t j = 0; j < subrecords.size(); j++)
                {
                    std::cout << "\t\t" << subrecords[j].key << "\t\t" << subrecords[j].name << "\t\t" << subrecords[j].genre_code << "\t\t" << subrecords[j].isbn << std::endl;
                }
            }

            std::cout << std::endl << std::endl;
        }
    }
}

// Відображає записи з slave-file
static void UtS()
{
    std::ifstream booksFileForRead("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!booksFileForRead.is_open())
    {
        booksFileForRead = std::ifstream("booksFile.fl", std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!booksFileForRead.is_open())
        {
            std::cerr << "Unable to open file booksFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file booksFile.fl");
        }
    }

    booksFileForRead.seekg(sizeof(uint32_t), std::ios::beg);

    std::cout << "ID\tName\t\t\t\tGenre Code\tISBN" << std::endl;

    Book tmp;

    while (true) {
        booksFileForRead.read(reinterpret_cast<char*>(&tmp), sizeof(Book));
        if (booksFileForRead.eof()) {
            break;
        }
        std::cout << tmp.key << "\t"
            << tmp.name << "\t\t"
            << static_cast<int>(tmp.genre_code) << "\t\t"
            << tmp.isbn << std::endl;
    }
}

static void CreateFiles()
{
    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Genre" + std::to_string(i);
        Genre f(i, name.c_str());
        InsertM(f);
    }

    for (size_t i = 1; i < 50; i++)
    {
        std::string name = "Book" + std::to_string(i);
        // uint32_t gc = rand() % (50 - 1 + 1) + 1;
        // Book f(i, gc, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, name.c_str());
        Book f(i, i, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, name.c_str());
        InsertS(f);
    }
}

int main()
{
    // Ввід 5 master-записів.
    for (size_t i = 1; i <= 5; i++)
    {
        std::string name = "Genre" + std::to_string(i);
        Genre newGenre = { name.c_str() };
        InsertM(newGenre);
    }

    // Для 3-х master-записів ввести 1, 2 та підлеглі записи.
    Book tmp = { 1, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book1 with genre1" };
    InsertS(tmp);
    tmp = { 1, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book2 with genre1" };
    InsertS(tmp);
    tmp = { 2, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book1 with genre2" };
    InsertS(tmp);
    tmp = { 2, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book2 with genre2" };
    InsertS(tmp);
    tmp = { 3, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book1 with genre3" };
    InsertS(tmp);
    tmp = { 3, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book2 with genre3" };
    InsertS(tmp);
    tmp = { 3, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book3 with genre3" };
    InsertS(tmp);
    tmp = { 4, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book1 with genre4" };
    InsertS(tmp);
    tmp = { 4, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, "Book2 with genre4" };
    InsertS(tmp);

    // ut-m, ut-s.
    std::cout << "Command used: UT-M" << std::endl;
    UtM();
    std::cout << std::endl;
    std::cout << "Command used: UT-S" << std::endl;
    UtS();
    std::cout << std::endl << std::endl;

    // Вилучити master-запис з двома підлеглими.
    DelM(1);
    DelM(3);
    DelS(3);

    // ut-m, ut-s.
    std::cout << "Command used: UT-M" << std::endl;
    UtM();
    std::cout << std::endl;
    std::cout << "Command used: UT-S" << std::endl;
    UtS();
    std::cout << std::endl << std::endl;

    // Ввід ще одного master-запису та підлеглого до нього запису.
    std::cout << "Creating new master record!" << std::endl
        << "Input new genre name: " << std::endl;
    std::string genrename;
    std::cin >> genrename;
    Genre newGenre(genrename.c_str());
    InsertM(newGenre);
    std::cout << "Master record creating is finished." << std::endl << std::endl;

    std::cout << "Creating new slave record!" << std::endl
        << "Input new book name: " << std::endl;
    std::string bookname;
    std::cin >> bookname;
    std::cout << "Input new book Genre: " << std::endl;
    int bookGenreId;
    std::cin >> bookGenreId;
    Book newBook(bookGenreId, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, bookname.c_str());
    InsertS(newBook);
    std::cout << "Slave record creating is finished." << std::endl << std::endl;

    // ut-m, ut-s.
    std::cout << "Command used: UT-M" << std::endl;
    UtM();
    std::cout << std::endl;
    std::cout << "Command used: UT-S" << std::endl;
    UtS();
    std::cout << std::endl << std::endl;

    // Оновлення записів
    std::cout << "Update master record!" << std::endl
        << "Input ID of record you want to update: " << std::endl;
    int genreId;
    std::cin >> genreId;
    if (genreId == NULL)
    {
        std::cout << "Please input integer" << std::endl;
        std::cin >> genreId;
    }
    std::cout << "Input new genre name: " << std::endl;
    std::cin >> genrename;
    Genre updatedGenre(genrename.c_str());
    UpdateM(genreId, updatedGenre);
    std::cout << "Master record updating is finished." << std::endl << std::endl;

    std::cout << "Update slave record!" << std::endl
        << "Input ID of record you want to update: " << std::endl;
    int bookId;
    std::cin >> bookId;
    if (bookId == NULL)
    {
        std::cout << "Please input integer" << std::endl;
        std::cin >> bookId;
    }
    std::cout << "Input new book name: " << std::endl;
    std::cin >> bookname;
    Book updatedBook(bookId, (uint32_t)rand() % (99999999 - 10000000 + 1) + 10000000, bookname.c_str());
    UpdateS(bookId, updatedBook);
    std::cout << "Slave record updating is finished." << std::endl << std::endl;

    // ut-m, ut-s.
    std::cout << "Command used: UT-M" << std::endl;
    UtM();
    std::cout << std::endl;
    std::cout << "Command used: UT-S" << std::endl;
    UtS();
    std::cout << std::endl << std::endl;

    return 0;
}
