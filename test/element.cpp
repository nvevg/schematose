//   Copyright 2020-2021 github.com/nvevg
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <gtest/gtest.h>

#include <schematose.hpp>

using Backends = testing::Types<schematose::tinyxml2_backend>;

template<typename T>
struct ElementTestSuite : public ::testing::Test {};
TYPED_TEST_SUITE_P(ElementTestSuite);

namespace
{
    template <typename Schema, typename Backend>
    bool Validate(const char *xml)
    {
        ::tinyxml2::XMLDocument doc;
        auto error = doc.Parse(xml);
        if (error != ::tinyxml2::XML_SUCCESS)
            throw nullptr;

        return schematose::validator_for<Schema>::template validate_with<Backend>(doc);
    }
}

using namespace schematose;

TYPED_TEST_P(ElementTestSuite, ValidateElementTreeTest) {
    using Schema = element<"Root",
                        element<"Child",
                            element<"ChildChild">,
                            element<"MultiChild",
                                element<"A",
                                    element<"B">,
                                        element<"C">
                                    >
                                >
                            >
                        >;

    static constexpr auto *XML = "<Root>"
                                                    "<Child>"
                                                    "   <ChildChild />"
                                                    "   <MultiChild>"
                                                    "       <A>"
                                                    "           <B>"
                                                    "               <C/>"
                                                    "           </B>"
                                                    "       </A>"
                                                    "   </MultiChild>"
                                                    "</Child>"
                                               "</Root>";

    GTEST_ASSERT_TRUE((Validate<Schema, TypeParam>(XML)));
}

TYPED_TEST_P(ElementTestSuite, False_On_WrongName)
{
    using Schema = element<"Root",
                        element<"Child">>;

    static constexpr auto *XML =
                                 "<Root>"
                                 "  <Child1 />"
                                 "</Root>";
    GTEST_ASSERT_FALSE((Validate<Schema, TypeParam>(XML)));
}

TYPED_TEST_P(ElementTestSuite, False_On_ExtraChildren)
{
    using Schema = element<"Root",
                        element<"Child">>;

    static constexpr auto *XML =
            "<Root>"
            "   <Child>"
            "       <ChildChild1 />"
            "   </Child>"
            "</Root>";
    GTEST_ASSERT_TRUE((Validate<Schema, TypeParam>(XML)));
}

REGISTER_TYPED_TEST_SUITE_P(ElementTestSuite, ValidateElementTreeTest, False_On_WrongName, False_On_ExtraChildren);
INSTANTIATE_TYPED_TEST_SUITE_P(Element, ElementTestSuite, Backends);