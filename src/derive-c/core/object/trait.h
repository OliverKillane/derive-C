#pragma once

#define TRAIT_CLONEABLE(SELF) REQUIRE_METHOD(SELF, SELF, clone, (SELF const*));

#define TRAIT_DELETABLE(SELF) REQUIRE_METHOD(void, SELF, delete, (SELF*));
