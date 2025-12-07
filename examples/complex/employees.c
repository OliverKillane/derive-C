/// @file
/// @example complex/employees.c
/// @brief Composing arenas, maps & vectors
/// Demonstrating embedding vectors inside hashmaps, and making efficient use of small indexes for
/// arenas.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/alloc/std.h>
#include <derive-c/core/prelude.h>
#include <derive-c/utils/for.h>

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

void name_debug(const name* self, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "name@%p { forename: \"%s\", surname: \"%s\" }", self, self->forename,
            self->surname);
}

typedef struct {
    int value;
} age;

bool age_eq(age const* age_1, age const* age_2) { return age_1->value == age_2->value; }
size_t age_hash(age const* age) { return age->value; }
void age_debug(age const* self, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "%d years", self->value);
}

typedef struct {
    name name;
    char const* email;
    age age;
} employee;

void employee_debug(employee const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, "employee@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);

    debug_fmt_print(fmt, stream, "name: ");
    name_debug(&self->name, fmt, stream);
    fprintf(stream, ",\n");

    debug_fmt_print(fmt, stream, "email: \"%s\",\n", self->email);

    debug_fmt_print(fmt, stream, "age: ");
    age_debug(&self->age, fmt, stream);
    fprintf(stream, ",\n");

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

#define INDEX_BITS 16
#define VALUE employee
#define VALUE_DEBUG employee_debug
#define NAME employees
#include <derive-c/container/arena/contiguous/template.h>

#define ITEM employees_index_t
#define ITEM_DEBUG employees_index_t_debug
#define NAME same_age_employees
#include <derive-c/container/vector/dynamic/template.h>

#define KEY age
#define KEY_EQ age_eq
#define KEY_HASH age_hash
#define KEY_DEBUG age_debug
#define VALUE same_age_employees
#define VALUE_DEBUG same_age_employees_debug
#define NAME employees_by_age
#include <derive-c/container/map/decomposed/template.h>

typedef struct {
    employees data;
    employees_by_age by_age;
} hr_system;

hr_system hr_system_new() {
    return (hr_system){
        .data = employees_new_with_capacity_for(1000, stdalloc_get()),
        .by_age = employees_by_age_new(stdalloc_get()),
    };
}

void hr_system_new_employee(hr_system* self, employee emp) {
    printf("Adding employee %s %s\n", emp.name.forename, emp.name.surname);
    employees_index_t idx = employees_insert(&self->data, emp);
    same_age_employees* idxes = employees_by_age_try_write(&self->by_age, emp.age);
    if (!idxes) {
        idxes =
            employees_by_age_insert(&self->by_age, emp.age, same_age_employees_new(stdalloc_get()));
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
    employees_index_t const* idx =
        same_age_employees_read(idxes, same_age_employees_size(idxes) - 1);
    return employees_read(&self->data, *idx);
}

void hr_system_debug(hr_system const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, "hr_system@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);

    debug_fmt_print(fmt, stream, "data: ");
    employees_debug(&self->data, fmt, stream);
    fprintf(stream, ",\n");

    debug_fmt_print(fmt, stream, "by_age: ");
    employees_by_age_debug(&self->by_age, fmt, stream);
    fprintf(stream, ",\n");

    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

void hr_system_delete(hr_system* self) {
    employees_delete(&self->data);

    FOR(employees_by_age, &self->by_age, iter, entry) { same_age_employees_delete(entry.value); }

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
    ASSERT(newest_22);
    ASSERT(name_eq(&newest_22->name, &bob_name));

    hr_system_debug(&hr, debug_fmt_new(), stdout);

    hr_system_delete(&hr);
}
