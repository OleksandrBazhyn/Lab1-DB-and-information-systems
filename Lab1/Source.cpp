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
    long position; // � ���� key * sizeof(Genre) + sizeof(uint32_t)
};

static Genre& GetM(uint32_t GenreKey) { // ����������� �������� �������. �������� ���� ����, ���� ���� ������ � fl ���� � ������� ��'��� �����
    std::ifstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }

    // ������� ������ � �����
    IndexGenre currentIndex;
    bool found = false;

    while (indexGenresFile.read(reinterpret_cast<char*>(&currentIndex), sizeof(IndexGenre))) {
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

        // ������� �� ������� ������ � ��������� ����
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
    // ������ ����� ����� � �������� ����
    std::fstream genresFile("genresFile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresFile.is_open()) {
        std::cerr << "Unable to open file genresFile.fl" << std::endl;
        throw std::runtime_error("Unable to open file genresFile.fl");
    }

    // ĳ������� ������� ������ � ����. �� ���������� �� ������� ����� (header)
    genresFile.seekg(0, std::ios::beg);
    uint32_t recordsCount = 0;
    genresFile.read(reinterpret_cast<char*>(&recordsCount), sizeof(uint32_t));

    genresFile.seekp(0, std::ios::end);  // ������� � ������ �������

    recordsCount++;
    newGenre.key = recordsCount;

    std::streampos newPosition = genresFile.tellp();  // ������ ������� ������ ������

    genresFile.write(reinterpret_cast<const char*>(&newGenre), sizeof(Genre));

    genresFile.seekp(0, std::ios::beg);
    genresFile.write(reinterpret_cast<const char*>(&recordsCount), sizeof(uint32_t)); // ������ ����� ����� �� �������� recordsCount � ���� �� �������

    genresFile.flush();
    genresFile.close();

    // ��������� �������� �������
    std::fstream indexGenresFile("indexGenresFile.ind", std::ios::binary | std::ios::in | std::ios::out | std::ios::ate);

    if (!indexGenresFile.is_open()) {
        std::cerr << "Unable to open file indexGenresFile.ind" << std::endl;
        throw std::runtime_error("Unable to open file indexGenresFile.ind");
    }
    // indexGenresFile.seekg(0, std::ios::end);     � std::ios::ate �������� � ��� ����� ������� ���� � ����
    // 
    // �������� ����� ����� (�������� ������� ������� � ���� �����)
    std::streampos fileSize = indexGenresFile.tellg();

    // ��������� ������� ������ � ����
    uint32_t indexRecordsCount = fileSize / sizeof(IndexGenre);

    // ������� �� ���������� ������
    indexGenresFile.seekg(-static_cast<std::streamoff>(sizeof(IndexGenre)), std::ios::cur);

    // ��������� ������� �����
    IndexGenre lastIndex;
    indexGenresFile.read(reinterpret_cast<char*>(&lastIndex), sizeof(IndexGenre));

    // �������� ������� ����
    uint32_t lastKey = indexRecordsCount > 0 ? lastIndex.key : 0;

    // �������� ���� ��� ������ ������
    uint32_t newKey = lastKey + 1;

    // ������ ����� ����� � ������
    IndexGenre newIndex;
    newIndex.key = newKey;
    newIndex.position = newPosition;

    indexGenresFile.seekp(0, std::ios::end);  // ������� � ������ �������
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
    /*std::fstream genresfile("genresfile.fl", std::ios::binary | std::ios::in | std::ios::out);

    if (!genresfile.is_open()) {
        std::cerr << "Unable to open file genresfile.fl" << std::endl;
        return 1;
    }*/








    const std::string filename = "file.bin";

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    auto err = errno;

    if (err == ENOENT)
    {
        file = std::fstream(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    }

    if (!file) {
        std::cerr << "Unable to open file=" << filename << std::endl;

        return -1;
    }


    return 0;
}

