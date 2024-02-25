#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
    long position; // � ���� key * sizeof(Genre) + sizeof(uint32_t)
};

static std::vector<Genre> GetM(uint32_t GenreKey) { // ����������� �������� �������. �������� ���� ����, ���� ���� ������ � fl ���� � ������� ��'��� �����
    std::ifstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in);
    
    if (!indexGenresFile.is_open()) {
        std::ofstream createIndexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::trunc);
        if (!indexGenresFile.is_open()) {
            std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
            throw std::runtime_error("Unable to open file indexGenresFile.ind");
        }
        createIndexGenresFile.close();
        indexGenresFile = std::ifstream("indexGenresFile.ind", std::ios::binary | std::ios::in);
    }

    // ������� ������ � �����
    IndexGenre currentIndex;
    std::vector<IndexGenre> neededIndexesGenre = {};
    bool foundAll = false;

    indexGenresFile.seekg(0, std::ios::beg);

    while (true) {
        if (!indexGenresFile.eof())
        {
            indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre));
            if (currentIndex.key == GenreKey) {
                neededIndexesGenre.push_back(currentIndex);
                foundAll = true;
            }
        }
        else {
            break;
        }
    }

    indexGenresFile.close();

    if (foundAll) {
        std::ifstream genresFile("genresFile.fl", std::ios::binary | std::ios::in);
        if (!genresFile.is_open()) {
            std::cerr << "Unable to open file genresFile.fl" << std::endl;
            throw std::runtime_error("Unable to open file genresFile.fl");
        }

        std::vector<Genre> neededGenres(neededIndexesGenre.size());

        for (size_t i = 0; i < neededIndexesGenre.size(); i++)
        {
            // ������� �� ������� ������ � ��������� ����
            genresFile.seekg(neededIndexesGenre[i].position, std::ios::beg);

            genresFile.read(reinterpret_cast<char*>(&neededGenres[i]), sizeof(Genre));

            genresFile.close();
        }
        return neededGenres;
    }
    else {
        std::cerr << "Genre not found" << std::endl;
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

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
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

    genresFile.seekp(0, std::ios::end);  // ������� � ������ �������

    recordsCount++;
    newGenre.key = recordsCount;

    std::streampos newPosition = genresFile.tellp();  // ������ ������� ������ ������

    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������ ����� ����� � ��������� recordsCount �� �������

    genresFile.flush();
    genresFile.close();

    // ��������� �������� �������
    std::ifstream indexGenresFileForRead("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFileForRead.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    indexGenresFileForRead.seekg(0, std::ios::end);

    // �������� ����� ����� (�������� ������� ������� � ���� �����)
    std::streampos endFilePos = indexGenresFileForRead.tellg();

    // ��������� ������� ������ � ����
    uint32_t indexRecordsCount = endFilePos / sizeof(IndexGenre);

    // ������� �� ���������� ������
    indexGenresFileForRead.seekg(-static_cast<std::streamoff>(sizeof(IndexGenre)), std::ios::cur);

    // ��������� ������� �����
    IndexGenre lastIndex;
    indexGenresFileForRead.read(reinterpret_cast<char*>(&lastIndex), sizeof(IndexGenre));

    indexGenresFileForRead.close();

    // �������� ������� ����
    uint32_t lastKey = indexRecordsCount > 0 ? lastIndex.key : 0;

    // �������� ���� ��� ������ ������
    uint32_t newKey = lastKey + 1;

    // ������ ����� ����� � ������
    IndexGenre newIndex;
    newIndex.key = newKey;
    newIndex.position = newPosition;

    std::ofstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::app);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }

    indexGenresFile.seekp(0, std::ios::end);  // ������� � ������ �������
    indexGenresFile.write(reinterpret_cast<const char*>(&newIndex), sizeof(IndexGenre));

    indexGenresFile.flush();
    indexGenresFile.close();
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

    Genre newGenre = {0, "new Genre1"};
    AddGenre(newGenre);
    Genre newGenre2 = { 1, "new Genre2" };
    AddGenre(newGenre2);

    // ��������� GetM
    uint32_t genreKeyToFind = 1;
    std::vector<Genre> res = GetM(genreKeyToFind);

    std::cout << res[0].key;

    return 0;
}

