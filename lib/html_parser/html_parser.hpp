/*
 * Copyright (c) 2017 SPLI (rangerlee@foxmail.com)
 * Latest version available at: http://github.com/rangerlee/htmlparser.git
 *
 * A Simple html parser.
 * More information can get from README.md
 *
 */

#ifndef HTMLPARSER_HPP_
#define HTMLPARSER_HPP_

#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <memory>

using std::enable_shared_from_this;
using std::shared_ptr;
using std::weak_ptr;

/**
 * class HtmlElement
 * HTML Element struct
 */
class HtmlElement : public enable_shared_from_this<HtmlElement> {
public:
    friend class HtmlParser;
    friend class HtmlDocument;

public:
    typedef std::vector<shared_ptr<HtmlElement>>::const_iterator ChildIterator;
    const ChildIterator ChildBegin() { return children.begin(); }
    const ChildIterator ChildEnd() { return children.end(); }

    typedef std::map<std::string, std::string>::const_iterator AttributeIterator;
    const AttributeIterator AttributeBegin() { return attribute.begin(); }
    const AttributeIterator AttributeEnd() { return attribute.end(); }

public:
    HtmlElement() {}
    HtmlElement(shared_ptr<HtmlElement> p) : parent(p) {}

    std::string GetAttribute(const std::string &k) {
        if (attribute.find(k) != attribute.end()) return attribute[k];
        return "";
    }

    shared_ptr<HtmlElement> GetElementById(const std::string &id) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if ((*it)->GetAttribute("id") == id) return *it;
            shared_ptr<HtmlElement> r = (*it)->GetElementById(id);
            if (r) return r;
        }
        return shared_ptr<HtmlElement>();
    }

    std::vector<shared_ptr<HtmlElement>> GetElementByClassName(const std::string &name) {
        std::vector<shared_ptr<HtmlElement>> result;
        GetElementByClassName(name, result);
        return result;
    }

    std::vector<shared_ptr<HtmlElement>> GetElementByTagName(const std::string &name) {
        std::vector<shared_ptr<HtmlElement>> result;
        GetElementByTagName(name, result);
        return result;
    }

    void SelectElement(const std::string& rule, std::vector<shared_ptr<HtmlElement>>& result);

    shared_ptr<HtmlElement> GetParent() { return parent.lock(); }

    const std::string &GetValue() {
        if (value.empty() && children.size() == 1 && children[0]->GetName() == "plain") {
            return children[0]->GetValue();
        }
        return value;
    }

    const std::string &GetName() { return name; }

    std::string text() {
        std::string str;
        PlainStylize(str);
        return str;
    }

    std::string html() {
        std::string str;
        HtmlStylize(str);
        return str;
    }

private:
    void PlainStylize(std::string& str) {
        if (name == "head" || name == "meta" || name == "style" || name == "script" || name == "link") {
            return;
        }

        if (name == "plain") {
            str.append(value);
            return;
        }

        for (size_t i = 0; i < children.size();) {
            children[i]->PlainStylize(str);

            if (++i < children.size()) {
                std::string ele = children[i]->GetName();
                if (ele == "td") {
                    str.append("\t");
                }
                else if (ele == "tr" || ele == "br" || ele == "div" || ele == "p" || ele == "hr" || ele == "area" ||
                  ele == "h1" || ele == "h2" || ele == "h3" || ele == "h4" || ele == "h5" || ele == "h6" || ele == "h7") {
                    str.append("\n");
                }
            }
        }
    }

    void HtmlStylize(std::string& str) {
        if (name.empty()) {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }
            return;
        }
        else if (name == "plain") {
            str.append(value);
            return;
        }

        str.append("<" + name);
        for (auto it = attribute.begin(); it != attribute.end(); ++it) {
            str.append(" " + it->first + "=\"" + it->second + "\"");
        }
        str.append(">");

        if (children.empty()) {
            str.append(value);
        } else {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }
        }

        str.append("</" + name + ">");
    }

private:
    void GetElementByClassName(const std::string &name, std::vector<shared_ptr<HtmlElement>> &result) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            std::set<std::string> attr_class = SplitClassName((*it)->GetAttribute("class"));
            std::set<std::string> class_name = SplitClassName(name);

            auto iter = class_name.begin();
            for (; iter != class_name.end(); ++iter) {
                if (attr_class.find(*iter) == attr_class.end()) break;
            }

            if (iter == class_name.end()) InsertIfNotExists(result, *it);

            (*it)->GetElementByClassName(name, result);
        }
    }

    void GetElementByTagName(const std::string &name, std::vector<shared_ptr<HtmlElement>> &result) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if ((*it)->name == name) InsertIfNotExists(result, *it);
            (*it)->GetElementByTagName(name, result);
        }
    }

    void GetAllElement(std::vector<shared_ptr<HtmlElement>> &result) {
        for (size_t i = 0; i < children.size(); ++i) {
            InsertIfNotExists(result, children[i]);
            children[i]->GetAllElement(result);
        }
    }

    void Parse(const std::string &attr);

    static std::set<std::string> SplitClassName(const std::string& name);
    static void InsertIfNotExists(std::vector<shared_ptr<HtmlElement>>& vec, const shared_ptr<HtmlElement>& ele);

private:
    std::string name;
    std::string value;
    std::map<std::string, std::string> attribute;
    weak_ptr<HtmlElement> parent;
    std::vector<shared_ptr<HtmlElement>> children;
};

/**
 * class HtmlDocument
 */
class HtmlDocument {
public:
    HtmlDocument(shared_ptr<HtmlElement> &root) : root_(root) {}

    shared_ptr<HtmlElement> GetElementById(const std::string &id) { return root_->GetElementById(id); }
    std::vector<shared_ptr<HtmlElement>> GetElementByClassName(const std::string &name) { return root_->GetElementByClassName(name); }
    std::vector<shared_ptr<HtmlElement>> GetElementByTagName(const std::string &name) { return root_->GetElementByTagName(name); }

    std::vector<shared_ptr<HtmlElement>> SelectElement(const std::string& rule){
        std::vector<shared_ptr<HtmlElement>> result;
        for (auto it = root_->ChildBegin(); it != root_->ChildEnd(); ++it) {
            (*it)->SelectElement(rule, result);
        }
        return result;
    }

    std::string html() { return root_->html(); }
    std::string text() { return root_->text(); }

private:
    shared_ptr<HtmlElement> root_;
};

/**
 * class HtmlParser
 */
class HtmlParser {
public:
    HtmlParser() {
        static const std::string token[] = { "br", "hr", "img", "input", "link", "meta",
            "area", "base", "col", "command", "embed", "keygen", "param", "source", "track", "wbr"};
        self_closing_tags_.insert(token, token + sizeof(token)/sizeof(token[0]));
    }

    shared_ptr<HtmlDocument> Parse(const char *data, size_t len);
    shared_ptr<HtmlDocument> Parse(const std::string &data) { return Parse(data.data(), data.size()); }

private:
    size_t ParseElement(size_t index, shared_ptr<HtmlElement> &element);
    size_t SkipUntil(size_t index, const char *data);
    size_t SkipUntil(size_t index, const char data);

private:
    const char *stream_;
    size_t length_;
    std::set<std::string> self_closing_tags_;
    shared_ptr<HtmlElement> root_;
};

#endif
