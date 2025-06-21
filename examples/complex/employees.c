#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <derive-c/macros/iterators.h>

typedef struct {
    char const* forename;
    char const* surname;
} name;

bool name_eq(const name* name_1, const name* name_2) {
    if (!name_1 || !name_2)
        return false;
    if (!name_1->forename || !name_2->forename)
        return false;
    if (!name_1->surname || !name_2->surname)
        return false;

    return strcmp(name_1->forename, name_2->forename) == 0 &&
           strcmp(name_1->surname, name_2->surname) == 0;
}

typedef struct {
    int value;
} age;

typedef struct {
    name name;
    char const* email;
    age age;
} employee;

bool age_eq(age const* age_1, age const* age_2) { return age_1->value == age_2->value; }
size_t age_hash(age const* age) { return age->value; }

#define INDEX_BITS 16
#define V employee
#define SELF employees
#include <derive-c/structures/arena/template.h>

#define T employees_index
#define SELF same_age_employees
#include <derive-c/structures/vector/template.h>

#define K age
#define V same_age_employees
#define EQ age_eq
#define HASH age_hash
#define SELF employees_by_age
#include <derive-c/structures/hashmap/template.h>

typedef struct {
    employees data;
    employees_by_age by_age;
} hr_system;

hr_system hr_system_new() {
    return (hr_system){
        .data = employees_new_with_capacity_for(1000),
        .by_age = employees_by_age_new(),
    };
}

void hr_system_new_employee(hr_system* self, employee emp) {
    printf("Adding employee %s %s\n", emp.name.forename, emp.name.surname);
    employees_index idx = employees_insert(&self->data, emp);
    same_age_employees* idxes = employees_by_age_try_write(&self->by_age, emp.age);
    if (!idxes) {
        idxes = employees_by_age_insert(&self->by_age, emp.age, same_age_employees_new());
    }
    same_age_employees_push(idxes, idx);
}

employee const* hr_system_newest_of_age(hr_system const* self, age age) {
    same_age_employees const* idxes = employees_by_age_try_read(&self->by_age, age);
    if (!idxes) {
        return NULL;
    }
    if (same_age_employees_size(idxes) == 0) {
        return NULL;
    }
    employees_index const* idx = same_age_employees_read(idxes, same_age_employees_size(idxes) - 1);
    return employees_read(&self->data, *idx);
}

void hr_system_delete(hr_system* self) {
    employees_delete(&self->data);

    employees_by_age_iter iter = employees_by_age_get_iter(&self->by_age);
    while (!employees_by_age_iter_empty(&iter)) {
        employees_by_age_kv kv = employees_by_age_iter_next(&iter);
        same_age_employees_delete(kv.value);
    }

    employees_by_age_delete(&self->by_age);
}

int main() {
    hr_system hr = hr_system_new();

    employee frank = {
        .age = (age){.value = 22},
        .email = "veryverylongemail@someprovider.net",
        .name =
            (name){
                .forename = "Frank",
                .surname = "Lee",
            },
    };
    hr_system_new_employee(&hr, frank);

    name bob_name = {
        .forename = "Bob",
        .surname = "Mike",
    };
    employee bob = {
        .age = (age){.value = 22},
        .email = "bib@cool.org",
        .name = bob_name,
    };
    hr_system_new_employee(&hr, bob);

    employee const* newest_22 = hr_system_newest_of_age(&hr, (age){.value = 22});
    assert(newest_22);
    assert(name_eq(&newest_22->name, &bob_name));

    hr_system_delete(&hr);
}
