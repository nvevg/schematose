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

#ifndef SCHEMATOSE_HPP
#define SCHEMATOSE_HPP
#include <cstring>
#include <string>
#include <string_view>
#include <iostream>
#include <type_traits>

#ifdef SCHEMATOSE_TIXML2_BACKEND
#include <tinyxml2.h>
#endif

#ifdef SCHEMATOSE_PUGIXML_BACKEND
// TODO: include pugixml
#endif

namespace schematose
{
    template <typename T>
    struct select_backend {};

    namespace details
    {
        /// Checks if all types in the parameter list are equal
        template <typename...>
        constexpr bool are_same_c = true;

        template <typename T, typename... Ts>
        constexpr bool are_same_c<T, Ts...> = std::conjunction_v<std::is_same<T, Ts>...>;

        template <typename CharT, std::size_t Len>
        struct basic_fixed_string
        {
            using char_type = CharT;

            constexpr static auto size = Len;

            constexpr basic_fixed_string(const CharT (&arr)[Len]) {
                std::copy(arr, arr + Len, data);
            }

            constexpr operator std::basic_string_view<CharT>() const noexcept {
                return std::string_view(data);
            }

            constexpr operator std::basic_string<char_type>() const noexcept {
                return std::string(data, size - 1);
            }

            constexpr std::size_t length() const noexcept
            {
                return Len;
            }

            CharT data[Len];
        };

        template<typename CharT, std::size_t N, std::size_t M>
        constexpr bool operator==(const basic_fixed_string<CharT, N> &lhs, const basic_fixed_string<CharT, M> &rhs) noexcept
        {
            if (lhs.length() != rhs.length())
                return false;

            for (std::size_t i = 0; i < N; ++i)
            {
                if (lhs.data[i] != rhs.data[i])
                    return false;
            }

            return true;
        }

        template <typename CharT, std::size_t N, std::size_t M>
        constexpr auto operator<=>(const basic_fixed_string<CharT, N> &lhs, const basic_fixed_string<CharT, M> &rhs) noexcept
        {
            for (std::size_t i = 0; i < std::min(N, M); ++i)
            {
                if (lhs.data[i] != rhs.data[i])
                    return lhs.data[i] <=> rhs.data[i];
            }
            return N <=> M;
        }

        template <typename Node>
        struct validator
        {
            template <typename LibraryTag, typename... ContextArgs>
            static bool constexpr validate_with(ContextArgs&&... args)
            {
                // Choose a user-provided version of make_backend
                // with ADL on argument's template parameter
                auto backend = make_backend(select_backend<LibraryTag>{},
                                            std::forward<ContextArgs>(args)...);

                Node n{};
                bool res = n.validate(backend);
                if (backend.is_valid_node() && res)
                    // TODO: throw - backend.is_valid_node() can't be TRUE after traversing the whole schema tree
                    return false;

                return res;
            }
        };
    }

    template <typename T>
    concept backend_like_c = requires (T x) {
        typename T::char_type;

        { x.move_first_child() } -> std::same_as<void>;
        { x.move_next_sibling() } -> std::same_as<void>;
        { x.move_parent() } -> std::same_as<void>;
        { x.is_valid_node() } -> std::same_as<bool>;
        { x.get_name() } -> std::convertible_to<std::basic_string<typename T::char_type>>;
    };

    template <details::basic_fixed_string Name, typename... Children>
    struct element: Children...
    {
        static constexpr auto name = Name;

        using name_type = decltype(Name);

        constexpr element() = default;

        constexpr element(const element& other) : Children(static_cast<const Children&>(other))... {}
        constexpr element(element&& other) noexcept : Children(static_cast<Children&&>(other))... {}

        constexpr element& operator=(const element& other) {
            ((static_cast<Children&>(*this) = static_cast<const Children&>(other)), ...);
            return *this;
        }

        constexpr element& operator=(element&& other) noexcept {
            ((static_cast<Children&>(*this) = static_cast<Children&&>(other)), ...);
            return *this;
        }

        template <backend_like_c BackendType>
        constexpr bool validate(BackendType &backend) const
        {
            if (!backend.is_valid_node())
                return false;

            if (backend.get_name() != std::string(name))
                return false;

            if (sizeof...(Children) == 0)
            {
                backend.move_next_sibling();
                return true;
            }

            // descend into the first child
            backend.move_first_child();

            bool valid = ((static_cast<const Children &>(*this)).validate(backend) && ...) && true;
            // the only possible case for having a valid node is when there are extra nodes on child level
            if (backend.is_valid_node()) {
                return false;
            }

            if (valid)
            {
                backend.move_parent();
                backend.move_next_sibling();
            }

            return valid;
        }
    };
    template <typename Node>
    struct validator
    {
        template <typename LibraryTag, typename... ContextArgs>
        static bool constexpr validate_with(ContextArgs&&... args)
        {
            // Choose a user-provided version of make_backend
            // with ADL on argument's template parameter
            auto backend = make_backend(select_backend<LibraryTag>{},
                                          std::forward<ContextArgs>(args)...);

            Node n{};
            return n.validate(backend);
        }
    };
    template <std::size_t From, std::size_t To, typename Validator>
    struct quantify: public virtual Validator
    {
        template <backend_like_c BackendType>
        constexpr bool validate(BackendType &backend) const
        {
            if (!backend.is_valid_node())
                return false;


            return true;
        }
    };

    template <typename ValidatorType>
    struct optional: private quantify<0, 1, ValidatorType>
    {
        using BaseType = quantify<0, 1, ValidatorType>;

        template <backend_like_c BackendType>
        constexpr bool validate(BackendType &backend) const
        {
            if (!backend.is_valid_node())
                return false;

            return static_cast<const BaseType &>(*this).validate(backend);
        }
    };

    template <typename NodeT>
    struct validator_for: details::validator<NodeT> {};

    namespace backends
    {
#ifdef SCHEMATOSE_TIXML2_BACKEND
        namespace tinyxml2
        {
            struct tag {};

            struct backend
            {
                using char_type = char;

                backend(const ::tinyxml2::XMLElement *startNode): node_(startNode)
                {
                    // TODO: throw if startNode == nullptr
                }

                void move_first_child()
                {
                    if (!node_)
                        return;

                    // preserve parent
                    parent_ = node_;

                    node_ = node_->FirstChildElement();
                }

                void move_next_sibling()
                {
                    if (!node_)
                        return;

                    node_ = node_->NextSiblingElement();
                }

                void move_parent()
                {
                    if (!node_)
                        return;

                    if (node_)
                        node_ = static_cast<const ::tinyxml2::XMLElement *>(node_->Parent());

                    if (!parent_)
                        return;

                    node_ = static_cast<const ::tinyxml2::XMLElement *>(parent_);
                }

                bool is_valid_node()
                {
                    return node_ != nullptr;
                }

                std::basic_string<char_type> get_name()
                {
                    // TODO: throw if !is_valid_node
                    return node_->Name();
                }

            private:

                const ::tinyxml2::XMLElement *parent_{};

                const ::tinyxml2::XMLElement *node_{};
            };

            backend make_backend(select_backend<tag>, const ::tinyxml2::XMLDocument &doc)
            {
                return backend(doc.FirstChildElement());
            }

            backend make_backend(select_backend<tag>, const ::tinyxml2::XMLElement *doc)
            {
                return backend(doc);
            }

        }
#endif
    }

    using tinyxml2_backend = backends::tinyxml2::tag;
}

#endif
