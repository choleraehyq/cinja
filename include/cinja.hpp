#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace cinja {
    
    namespace detail {
        
        class value {
        public:
            template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
            value(T item): _content(boost::lexical_cast<std::string>(item)), _isdigit(true) {
            }
            
            value(std::string &item): _content(item) {}
            
            value(std::string &&item): _content(item) {}
            
            value(std::vector<std::string> &item): _vector(item) {}
            
            value(std::vector<std::string> &&item): _vector(item) {}
            
            
            bool b() {
                bool retval{ true };
                if (_content.empty()) retval = false;
                if (_isdigit && boost::lexical_cast<double>(_content) == 0.0) retval = false;
                if (boost::algorithm::to_upper_copy(_content) == "FALSE") retval = false;
                return retval;
            }
            
            std::string s() {
                return _content;
            }
            
            std::vector<std::string> v() {
                return _vector;
            }
                     
        private:
            std::string _content;
            std::vector<std::string> _vector;
            bool _isdigit{false};
        };

        std::string substitude(std::string raw, std::map<std::string, std::unique_ptr<value>> &context) {
            for (std::map<std::string, std::unique_ptr<value>>::iterator s = context.begin(); s != context.end(); s++) {
                boost::regex r{ "{{[[:space:]]*" + s->first + "[[:space:]]*}}" };
                raw = boost::regex_replace(raw, r, s->second->s());
            }
            return raw;
        }
        
        class section {
        public:
            virtual ~section() 
            {}
            virtual std::string render(std::map<std::string, std::unique_ptr<value>> &context)
            {}
        };
        
        class forsection: public section {
        public:
            forsection(): section() {}
            void addChild(std::unique_ptr<section> &&p) {
                _children.emplace_back(std::forward<std::unique_ptr<section>>(p));
            } 
            void setIter(std::string &&iterName, std::string &&iterRange) {
                this->iterName = iterName;
                this->iterRange = iterRange;
            }
            std::string render(std::map<std::string, std::unique_ptr<value>> &context) override {
                std::string retval;
                for (auto item: context[iterRange]->v()) {
                    std::string cur;
                    
                    context.insert(std::make_pair(iterName, std::make_unique<value>(item)));
                    for (size_t i = 0; i < _children.size(); i++) {
                        cur += _children[i]->render(context);
                    }
                    substitude(cur, context);
                    context.erase(iterName);
                    
                    retval += cur;
                }
                return retval;
            }
        private:
            std::vector<std::unique_ptr<section>> _children;
            std::string iterName, iterRange;
        };
        
        class ifsection: public section {
        public:
            ifsection(): section()  {}
            void addChild(std::unique_ptr<section> &&p) {
                _children.emplace_back(std::forward<std::unique_ptr<section>>(p));
            } 
            void setCondition(std::string &&condition) {
                this->condition = condition;
            }
            std::string render(std::map<std::string, std::unique_ptr<value>> &context) override {
                std::string retval;
                if (context[condition]->b()) {
                    for (size_t i = 0; i < _children.size(); i++) {
                        retval += _children[i]->render(context);
                    }
                    substitude(retval, context);
                }
                else retval = "";
                return retval;
            }
        private:
            std::vector<std::unique_ptr<section>> _children;
            std::string condition;
        };
        
        class plainsection: public section {
        public:
            plainsection(std::string &&s): section(), _content(s) { 
            }     
            std::string render(std::map<std::string, std::unique_ptr<value>> &context) override {
                return substitude(_content, context);
            }
        private:
            std::string _content;
        };
        
        std::unique_ptr<section> _parse(std::string &raw, size_t &pos, std::string::size_type instrStart) {
            auto instrEnd{ raw.find("%}", instrStart) };
            std::string instr{raw.substr(instrStart+2, instrEnd-instrStart-2)};
            std::vector<std::string> v;
            boost::algorithm::split(v, instr, boost::algorithm::is_space());
            v.erase(std::remove_if(v.begin(), v.end(), [](std::string &s) {
                return s.empty();
            }), v.end());
            std::unique_ptr<section> ret;
            
            if (v[0] == "for") {
                std::string iterName{v[1]};
                std::string iterRange{v[3]};  
                auto nextInstrStart{ raw.find("{%", instrEnd) };
                auto nextInstrEnd{ raw.find("%}", nextInstrStart) };
                auto retval{ std::make_unique<forsection>() };
                retval->setIter(std::move(iterName), std::move(iterRange));
                retval->addChild(std::make_unique<plainsection>(raw.substr(instrEnd+2, nextInstrStart-instrEnd-2)));
                
                while (boost::algorithm::trim_copy(
                        raw.substr(nextInstrStart+2, nextInstrEnd-nextInstrStart-2)) != "endfor") {
                    retval->addChild(_parse(raw, pos, nextInstrStart));
                    nextInstrStart = raw.find("{%", pos);
                    if (nextInstrStart > pos)
                        retval->addChild(std::make_unique<plainsection>(raw.substr(pos, nextInstrStart-pos)));
                    nextInstrEnd = raw.find("%}", pos);           
                }
                
                pos = nextInstrEnd + 2;
                ret = std::move(retval);  
            }
            else if (v[0] == "if") {
                std::string condition{v[1]};
                auto nextInstrStart{ raw.find("{%", instrEnd) };
                auto nextInstrEnd{ raw.find("%}", nextInstrStart) };
                auto retval{ std::make_unique<ifsection>() };
                retval->setCondition(std::move(condition));
                retval->addChild(std::make_unique<plainsection>(raw.substr(instrEnd+2, nextInstrStart-instrEnd-2)));
                
                while (boost::algorithm::trim_copy(
                        raw.substr(nextInstrStart+2, nextInstrEnd-nextInstrStart-2)) != "endif") {
                    
                    retval->addChild(_parse(raw, pos, nextInstrStart));
                    nextInstrStart = raw.find("{%", pos);
                    if (nextInstrStart > pos)
                        retval->addChild(std::make_unique<plainsection>(raw.substr(pos, nextInstrStart-pos)));
                    nextInstrEnd = raw.find("%}", pos);           
                }
                
                pos = nextInstrEnd + 2;
                ret = std::move(retval);
            }
            
            return ret;
        }
        
    }
    
    
    class Template {
    
    public:
        Template(std::string source): _body(std::move(source)) {
            parse(_body);
        };
        
        void parse(std::string &raw) {
            size_t pos = 0;
            auto size = raw.size();
            while (pos < size) {
                auto instrStart = raw.find("{%", pos);
                if (instrStart == std::string::npos) {
                    _sections.push_back(std::make_unique<detail::plainsection>(raw.substr(pos, raw.size())));
                    break;
                }
                _sections.push_back(std::make_unique<detail::plainsection>(raw.substr(pos, instrStart-pos)));
                _sections.push_back(detail::_parse(raw, pos, instrStart));
            }    
        }
        
        std::string render() {
            std::string retval;
            for (auto i = _sections.cbegin(); i !=  _sections.cend(); i++) {
                retval += (*i)->render(_context);
            }
            return retval;
        }
        
        template <typename T, typename = std::enable_if<std::is_arithmetic<T>::value>>
        void setValue(std::string name, T value) {
            _context.insert(std::make_pair(name, std::make_unique<detail::value>(value)));
        }
        
        void setValue(std::string name, std::string value) {
            _context.insert(std::make_pair(name, std::make_unique<detail::value>(value)));
        }
        
        void setValue(std::string name, std::vector<std::string> &value) {
            _context.insert(std::make_pair(name, std::make_unique<detail::value>(value)));
        }
        
        void setValue(std::string name, std::vector<std::string> &&value) {
            _context.insert(std::make_pair(name, std::make_unique<detail::value>(value)));
        } 
        
    private:
        std::string _body;
        std::vector<std::unique_ptr<detail::section>> _sections;
        std::map<std::string, std::unique_ptr<detail::value>> _context;
    };
}