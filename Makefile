# Root build for all implementations, the shared benchmark and the tests.
#
# Convention: every algorithm lives in implementations/<name>/ with
#   include/<header>.hpp      public interface
#   src/*.cpp                 implementation (compiled automatically)
#   tests/test_*.cpp          tests (built and run automatically)
# Shared code lives in common/ (interface + trial-division oracle) and bench/ (harness).
# A folder with no src/ directory yet (e.g. a placeholder) is simply skipped.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
BUILD    := build

IMPLS    := $(patsubst implementations/%/src,%,$(wildcard implementations/*/src))
INCLUDES := -Icommon $(foreach d,$(IMPLS),-Iimplementations/$(d)/include)

ALL_OBJS  :=
ALL_TESTS :=

define IMPL_RULES
OBJS_$(1)  := $$(patsubst implementations/$(1)/src/%.cpp,$(BUILD)/$(1)/%.o,$$(wildcard implementations/$(1)/src/*.cpp))
TESTS_$(1) := $$(patsubst implementations/$(1)/tests/%.cpp,$(BUILD)/$(1)/%,$$(wildcard implementations/$(1)/tests/test_*.cpp))
ALL_OBJS   += $$(OBJS_$(1))
ALL_TESTS  += $$(TESTS_$(1))

$(BUILD)/$(1)/%.o: implementations/$(1)/src/%.cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$(INCLUDES) -MMD -MP -c $$< -o $$@

$(BUILD)/$(1)/test_%: implementations/$(1)/tests/test_%.cpp $$(OBJS_$(1))
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$(INCLUDES) -MMD -MP $$^ -o $$@
endef
$(foreach d,$(IMPLS),$(eval $(call IMPL_RULES,$(d))))

.PHONY: all test bench clean list
all: $(BUILD)/bench $(ALL_TESTS)

$(BUILD)/bench: bench/bench.cpp $(ALL_OBJS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP $^ -o $@

# TEST_ARGS=--slow make test   runs the long test tiers where an implementation has them
test: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do echo "== $$t"; ./$$t $(TEST_ARGS) || exit 1; done

# ARGS="..." make bench   passes options to the harness (see README)
bench: $(BUILD)/bench
	@mkdir -p results
	./$(BUILD)/bench $(ARGS)

list:
	@echo "implementations with source: $(IMPLS)"

clean:
	rm -rf $(BUILD)

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
