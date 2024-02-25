#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <queue>
#include <map>

#define _CRT_SECURE_NO_DEPRECATE
#pragma warning (disable : 4996)

using namespace std;

struct Student {
    int id;
    char surname[20];
    int groupID;
    bool deleted = false;
    int next = -1;
};
struct Group {
    int id;
    char name[20];
    bool deleted = false;
};
struct Tindex {
    int id;
    int actualID;
};

Group get_m(int id, map<int, int>& groupIndex) {
    FILE* groups = fopen("groups.dat", "a+");
    Group g;
    if (groupIndex.empty()) return { -1 };
    auto m = groupIndex.rbegin();
    if (id > m->first || id < 1) {
        fclose(groups);
        return { -1 };
    }
    fseek(groups, (groupIndex[id] - 1) * sizeof(Group), SEEK_SET);
    if (fread(&g, sizeof(Group), 1, groups))
        fclose(groups);

    if (g.deleted == true) {
        return { -1 };
    }

    return g;
}

Student get_s(int id, map<int, int>& studentIndex) {
    FILE* students = fopen("students.dat", "a+");
    Student s;
    if (studentIndex.empty()) return { -1 };
    auto m = studentIndex.rbegin();
    if (id > m->first || id < 1) {
        fclose(students);
        return { -1 };
    }
    fseek(students, (studentIndex[id] - 1) * sizeof(Student), SEEK_SET);
    if (fread(&s, sizeof(Student), 1, students))
        fclose(students);

    if (s.deleted == true) {
        return { -1 };
    }

    return s;
}

void update_m(Group g, map<int, int>& groupIndex) {
    FILE* groups = fopen("groups.dat", "r+");
    int id = groupIndex[g.id];
    fseek(groups, (id - 1) * sizeof(Group), SEEK_SET);
    fwrite(&g, sizeof(Group), 1, groups);

    fclose(groups);
}

void update_s(Student s, map<int, int>& studentIndex, map<int, int>& GSIndex, int prevGroup) {
    FILE* students = fopen("students.dat", "r+");
    int id = studentIndex[s.id];

    if (s.groupID == prevGroup) {
        fseek(students, (id - 1) * sizeof(Student), SEEK_SET);
        fwrite(&s, sizeof(Student), 1, students);
        fclose(students);
        return;
    }
    //delete from group
    Student st = get_s(GSIndex[prevGroup], studentIndex);
    if (st.next == -1) { // if only one student in group
        GSIndex[prevGroup] = -1;
    }
    else if (st.id == s.id) { // if first student in group
        GSIndex[prevGroup] = s.next;
    }
    else {
        Student prev;
        while (st.id != s.id) {
            prev = st;
            st = get_s(st.next, studentIndex);
        }
        prev.next = st.next;
        fseek(students, (prev.id - 1) * sizeof(Student), SEEK_SET);
        fwrite(&prev, sizeof(Student), 1, students);
    }

    //insert to new group
    if (GSIndex[s.groupID] == -1) {
        GSIndex[s.groupID] = id;
    }
    else {
        Student lastStudent = get_s(GSIndex[s.groupID], studentIndex);
        while (lastStudent.next != -1) {
            lastStudent = get_s(lastStudent.next, studentIndex);
        }
        lastStudent.next = s.id;
        fseek(students, (lastStudent.id - 1) * sizeof(Student), SEEK_SET);
        fwrite(&lastStudent, sizeof(Student), 1, students);
    }

    s.next = -1;
    fseek(students, (id - 1) * sizeof(Student), SEEK_SET);
    fwrite(&s, sizeof(Student), 1, students);

    fclose(students);
}

void insert_m(map<int, int>& groupIndex, queue<Tindex>& deletedGroups, map<int, int>& GSIndex) {
    FILE* groups = fopen("groups.dat", "a+");
    Group newGroup = { 0, "", false };
    printf("Enter the name of the group\n");
    cin >> newGroup.name;
    if (groupIndex.empty()) {
        newGroup.id = 1;
    }
    else {
        auto m = groupIndex.rbegin();
        newGroup.id = m->first + 1;
    }

    if (!deletedGroups.empty()) {
        Tindex index = deletedGroups.front();
        deletedGroups.pop();
        groupIndex[newGroup.id] = index.actualID;
        update_m(newGroup, groupIndex);
    }
    else {
        fwrite(&newGroup, sizeof(Group), 1, groups);
        groupIndex[newGroup.id] = newGroup.id;
    }

    GSIndex[newGroup.id] = -1;
    fclose(groups);
}

void insert_s(map<int, int>& studentIndex, map<int, int>& GSIndex, queue<Tindex>& deletedStudents) {
    FILE* students = fopen("students.dat", "a+");
    Student newStudent;
    cout << "Enter the surname of the student\n";
    cin >> newStudent.surname;
    cout << "Enter the groupID of the student\n";
    cin >> newStudent.groupID;

    if (studentIndex.empty()) {
        newStudent.id = 1;
    }
    else {
        auto m = studentIndex.rbegin();
        //cout << m->first << " " << m->second << endl;
        newStudent.id = m->first + 1;
    }

    if (!deletedStudents.empty()) {
        Tindex index = deletedStudents.front();
        deletedStudents.pop();
        studentIndex[newStudent.id] = index.actualID;
        update_s(newStudent, studentIndex, GSIndex, newStudent.groupID);
    }
    else {
        fwrite(&newStudent, sizeof(Student), 1, students);
        studentIndex[newStudent.id] = newStudent.id;
    }

    if (GSIndex[newStudent.groupID] == -1) {
        GSIndex[newStudent.groupID] = newStudent.id;
    }
    else {
        Student lastStudent = get_s(GSIndex[newStudent.groupID], studentIndex);
        while (lastStudent.next != -1) {
            lastStudent = get_s(lastStudent.next, studentIndex);
        }
        lastStudent.next = newStudent.id;
        update_s(lastStudent, studentIndex, GSIndex, lastStudent.groupID);
    }

    fclose(students);
}

void delete_s(Student s, map<int, int>& studentIndex, map<int, int>& GSIndex, queue<Tindex>& deletedStudents) {
    FILE* students = fopen("students.dat", "r+");
    s.deleted = true;
    fseek(students, (studentIndex[s.id] - 1) * sizeof(Student), SEEK_SET);
    fwrite(&s, sizeof(Student), 1, students);

    if (GSIndex[s.groupID] == s.id) {
        GSIndex[s.groupID] = s.next;
    }
    else {
        Student prev;
        Student st = get_s(GSIndex[s.groupID], studentIndex);
        while (st.id != s.id) {
            prev = st;
            st = get_s(st.next, studentIndex);
        }
        prev.next = st.next;
        fseek(students, (prev.id - 1) * sizeof(Student), SEEK_SET);
        fwrite(&prev, sizeof(Student), 1, students);
    }

    deletedStudents.push({ s.id, studentIndex[s.id] });
    studentIndex.erase(s.id);

    /*for(auto x : studentIndex){
      cout << x.first << " " << x.second << endl;
    }*/

    fclose(students);
}

void delete_m(Group g, map<int, int>& groupIndex, map<int, int>& studentIndex, map<int, int>& GSIndex, queue<Tindex>& deletedGroups, queue<Tindex>& deletedStudents) {
    FILE* groups = fopen("groups.dat", "r+");
    g.deleted = true;
    fseek(groups, (groupIndex[g.id] - 1) * sizeof(Group), SEEK_SET);
    fwrite(&g, sizeof(Group), 1, groups);

    if (GSIndex[g.id] != -1) {
        Student lastStudent = get_s(GSIndex[g.id], studentIndex);
        if (lastStudent.next != -1) {
            while (lastStudent.next != -1) {
                delete_s(lastStudent, studentIndex, GSIndex, deletedStudents);
                lastStudent = get_s(lastStudent.next, studentIndex);
            }
        }
        delete_s(lastStudent, studentIndex, GSIndex, deletedStudents);
    }

    deletedGroups.push({ g.id, groupIndex[g.id] });

    groupIndex.erase(g.id);

    GSIndex.erase(g.id);

    fclose(groups);
}

void calc_m(queue<Tindex> deletedGroups) {
    FILE* groups = fopen("groups.dat", "a+");
    fseek(groups, 0L, SEEK_END);
    int n = ftell(groups) / sizeof(Group);
    n -= deletedGroups.size();
    cout << "The number of groups is " << n << endl;
    fclose(groups);
}

void calc_s(queue<Tindex> deletedStudents) {
    FILE* students = fopen("students.dat", "a+");
    fseek(students, 0L, SEEK_END);
    int n = ftell(students) / sizeof(Student);
    n -= deletedStudents.size();
    cout << "The number of students is " << n << endl;
    fclose(students);
}

void ut_m() {
    FILE* groups = fopen("groups.dat", "r");
    Group g;
    while (!feof(groups)) {
        if (fread(&g, sizeof(Group), 1, groups)) {
            cout << "id:" << g.id << " name:" << g.name << " deleted:" << g.deleted << endl;
        }
    }
}

void ut_s() {
    FILE* students = fopen("students.dat", "r");
    Student s;
    while (!feof(students)) {
        if (fread(&s, sizeof(Student), 1, students)) {
            cout << "id:" << s.id << " surname:" << s.surname << " group:" << s.groupID << " deleted:" << s.deleted << " next:" << s.next << endl;
        }
    }
}

void find_students(Group g, map<int, int>& studentIndex, map<int, int>& GSIndex) {
    FILE* students = fopen("students.dat", "a+");
    Student s;

    int id = GSIndex[g.id];
    if (id != -1) {
        s = get_s(id, studentIndex);
        while (s.id != -1) {
            cout << "id:" << s.id << " surname:" << s.surname << endl;
            s = get_s(s.next, studentIndex);
        }

    }

    fclose(students);
}

void read_index_table(map<int, int>& studentIndex, map<int, int>& groupIndex, map<int, int>& GSIndex) {
    Tindex index;
    FILE* ind = fopen("index_student.dat", "r+");
    while (!feof(ind)) {
        if (fread(&index, sizeof(Tindex), 1, ind)) {
            studentIndex[index.id] = index.actualID;
        }
    }
    fclose(ind);

    ind = fopen("index_group.dat", "r+");
    while (!feof(ind)) {
        if (fread(&index, sizeof(Tindex), 1, ind)) {
            groupIndex[index.id] = index.actualID;
        }
    }
    fclose(ind);

    ind = fopen("index_gs.dat", "r+");
    while (!feof(ind)) {
        if (fread(&index, sizeof(Tindex), 1, ind)) {
            GSIndex[index.id] = index.actualID;
        }
    }
    fclose(ind);
}

void write_index_table(map<int, int>& studentIndex, map<int, int>& groupIndex, map<int, int>& GSIndex) {
    FILE* ind = fopen("index_student.dat", "w");
    Tindex index;
    for (auto i : studentIndex) {
        index = { i.first, i.second };
        fwrite(&index, sizeof(Tindex), 1, ind);
    }
    fclose(ind);

    ind = fopen("index_group.dat", "w");
    for (auto i : groupIndex) {
        index = { i.first, i.second };
        fwrite(&index, sizeof(Tindex), 1, ind);
    }
    fclose(ind);

    ind = fopen("index_gs.dat", "w");
    for (auto i : GSIndex) {
        index = { i.first, i.second };
        fwrite(&index, sizeof(Tindex), 1, ind);
    }
    fclose(ind);
}

void find_deleted(queue<Tindex>& deletedGroups, queue<Tindex>& deletedStudents) {
    FILE* groups = fopen("groups.dat", "r");
    FILE* students = fopen("students.dat", "r");

    Group g;
    int counter = 0;
    while (!feof(groups)) {
        counter++;
        if (fread(&g, sizeof(Group), 1, groups))
            if (g.deleted == true) {
                deletedGroups.push({ g.id, counter });
            }
    }

    counter = 0;
    Student s;
    while (!feof(students)) {
        counter++;
        if (fread(&s, sizeof(Student), 1, students))
            if (s.deleted == true) {
                deletedStudents.push({ s.id, counter });
            }
    }
}

int main() {
    string command;
    Group g;
    Student s;
    map<int, int> studentIndex, groupIndex, GSIndex;
    int groupID, studentID;

    queue<Tindex> deletedStudents;
    queue<Tindex> deletedGroups;

    find_deleted(deletedGroups, deletedStudents);

    read_index_table(studentIndex, groupIndex, GSIndex);

    cout << "All commands: get_m, get_s, insert_m, insert_s, delete_m, delete_s, update_m, update_s, calc_m, calc_s, exit\n";

    while (true) {

        /*for(auto x : studentIndex){
          cout << x.first << " " << x.second << endl;
        }*/
        cout << "Enter the command\n";

        cin >> command;
        if (command == "get_m") {
            cout << "Enter the id of the group\n";
            cin >> groupID;
            g = get_m(groupID, groupIndex);
            if (g.id == -1) {
                printf("Group not found\n");
            }
            else {
                printf("id: %d Group: %s\n", g.id, g.name);
                printf("Students:\n");
                find_students(g, studentIndex, GSIndex);
            }
        }
        else if (command == "get_s") {
            cout << "Enter the id of the student\n";
            cin >> studentID;
            s = get_s(studentID, studentIndex);
            if (s.id == -1) {
                printf("Student not found\n");
            }
            else {
                printf("id: %d Student: %s\n", s.id, s.surname);
            }
        }
        else if (command == "insert_m") {
            insert_m(groupIndex, deletedGroups, GSIndex);
        }
        else if (command == "insert_s") {
            insert_s(studentIndex, GSIndex, deletedStudents);
        }
        else if (command == "delete_m") {
            cout << "Enter the id of the group\n";
            cin >> groupID;
            g = get_m(groupID, groupIndex);
            if (g.id == -1) {
                printf("Group not found\n");
                continue;
            }
            delete_m(g, groupIndex, studentIndex, GSIndex, deletedGroups, deletedStudents);
        }
        else if (command == "delete_s") {
            cout << "Enter the id of the student\n";
            cin >> studentID;
            s = get_s(studentID, studentIndex);
            if (s.id == -1) {
                printf("Student not found\n");
                continue;
            }
            delete_s(s, studentIndex, GSIndex, deletedStudents);
        }
        else if (command == "update_m") {
            cout << "Enter the id of the group\n";
            cin >> groupID;
            g = get_m(groupID, groupIndex);
            if (g.id == -1) {
                printf("Group not found\n");
                continue;
            }
            cout << "Enter the new name of the group\n";
            cin >> g.name;
            update_m(g, groupIndex);
        }
        else if (command == "update_s") {
            cout << "Enter the id of the student\n";
            cin >> studentID;
            s = get_s(studentID, studentIndex);
            if (s.id == -1) {
                printf("Student not found\n");
                continue;
            }
            groupID = s.groupID;
            cout << "Enter the new surname of the student\n";
            cin >> s.surname;
            cout << "Enter the new groupID of the student\n";
            cin >> s.groupID;

            update_s(s, studentIndex, GSIndex, groupID);
        }
        else if (command == "calc_m") {
            calc_m(deletedGroups);
        }
        else if (command == "calc_s") {
            calc_s(deletedStudents);
        }
        else if (command == "ut_m") {
            ut_m();
        }
        else if (command == "ut_s") {
            ut_s();
        }
        else if (command == "exit") {
            break;
        }
    }

    write_index_table(studentIndex, groupIndex, GSIndex);

    return 0;
}