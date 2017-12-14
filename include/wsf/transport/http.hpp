#ifndef WSF_TRANSPORT_HTTP_HPP
#define WSF_TRANSPORT_HTTP_HPP

#include <string>
#include <map>
#include <memory>
#include <functional>

#include <wsf/util/object.hpp>
#include <wsf/util/status_code.hpp>
#ifndef _WIN32
#include <string.h>
#endif


//forward declare

namespace wsf
{
	namespace transport
	{
		namespace http
		{
			struct request_t;
			struct response_t;

			typedef ::wsf::util::IObject IContext;
			typedef std::unique_ptr<IContext> context_ptr;
			typedef std::unique_ptr<request_t> request_ptr;
			typedef std::unique_ptr<response_t> response_ptr;

			///@note the context and request/response will be empty after the function invoked
			typedef std::function<void(context_ptr &, response_ptr &)> response_fn;
			typedef std::function<void(context_ptr &, request_ptr &)>  request_fn;
		}
	}
}

const std::string& get(const ::wsf::transport::http::request_t& req,
	const std::string& name,
	const std::string& default_value = "");

void set(::wsf::transport::http::request_t& req,
	const std::string& name, const std::string& value);

bool has(const ::wsf::transport::http::request_t& req, const std::string& name);

const std::string& get(const ::wsf::transport::http::response_t& res,
	const std::string& name, const std::string& default_value);

void set(::wsf::transport::http::response_t& res,
	const std::string& name, const std::string& value);

bool has(const ::wsf::transport::http::response_t& res, const std::string& name);

namespace wsf
{
    namespace transport
    {
        namespace http
        {
            //---------------------------------global interface--------------------------------------
            /**
			* @brief bind url base path to a dispatch function
			*   After success binding, the request message from
			*   remote endpoint will be dispatched by the 'fn'
			*
			* @example
			*       struct message_t
			*       {
			*           message_t(context_ptr &c, request_ptr &r)
			*               : context(c.release())
			*               , req(r.release())
			*           {
			*           }
			*           context_ptr context;
			*           request_ptr req;
			*       };
			*
			*       void post(context_ptr &context, request_ptr &req)
			*       {
			*           message_t m(context,req);
			*           Theron::Send(m, from, to);
			*       }
			*       //C++11 version
			*       auto post=[&actor](context_ptr &context, request_ptr &req)
			*       {
			*           message_t m(context,req);
			*           Theron::Send(m, from, actor.GetAdress());
			*       }
			*       Bind("http://127.0.0.1:8080/service-x",post);
			*
			* @return indicate initialize result
			*        -<em>false</em> fail
			*        -<em>true</em> succeed
			*/
            bool Bind(const std::string &url_base, request_fn fn);

			bool UnBind(const std::string &url_base);


            /**
			* @brief response to remote (for previous request )
			*   This function invoked, when application process previous
			*   request completed and then reply to remote endpoint
			*
			* @note context and res will be empty after invoked
			*
			* @example
			*
			* @param context   context of the corresponding request
			*
			* @param res   response message which to be replied
			*
			*/
            void ResponseToRemote(context_ptr &context, response_ptr &res);

            /**
			* @brief app send request to remote.
			*
			* @note context and req will be empty after invoked
			*
			* @example
			*
			* @param context   context of the corresponding request
			*
			* @param res   response message which to be replied
			*
			*/
			void RequestToRemote(context_ptr &context, request_ptr &req, response_fn fn);
			

            struct request_t
            {
                inline const std::string &host() const
                {
                    return host_;
                }
                inline std::string &host()
                {
                    return host_;
                }

                inline uint16_t port() const
                {
                    return port_;
                }
                inline uint16_t &port()
                {
                    return port_;
                }

                inline std::string &method()
                {
                    return method_;
                }
                inline const std::string &method() const
                {
                    return method_;
                }

                inline const std::string &path() const
                {
                    return path_;
                }
                inline std::string &path()
                {
                    return path_;
                }

                inline std::map<std::string, std::string> &query()
                {
                    return query_;
                }
                inline const std::map<std::string, std::string> &query() const
                {
                    return query_;
                }

                inline std::string &fragment()
                {
                    return fragment_;
                }
                inline const std::string &fragment() const
                {
                    return fragment_;
                }

                inline std::map<std::string, std::string> &header()
                {
                    return header_;
                }
                inline const std::map<std::string, std::string> &header() const
                {
                    return header_;
                }

                inline std::string &content()
                {
                    return content_;
                }
                inline const std::string &content() const
                {
                    return content_;
                }

				inline void set_url(const std::string& url)
				{
					size_t n = url.find_first_of("://");
					if (n == std::string::npos){ // no scheme


					}
					const char* begin = url.data();
					const char* end = begin + url.size();

					const char* s = NULL;
					s = strstr(begin, "://");
					if( s ){
						begin = s+3;
					}

					s = strchr(begin, '/');
					if( s ){//pick up path
						path_ = std::string(s);
					}
					end = s;
					s = begin;
					for (s = begin; s != end; s++)
					{
						if (*s == ':')
						{
							host_ = std::string(begin, s);
							port_ = (uint16_t)::std::stoul(std::string(s+1, end));
							return;
						}
					}
					host_ = std::string(begin, end);
					port_ = 80;

				}
            protected:
                std::string host_;
                uint16_t port_;
                std::string method_;
                std::string path_;
                std::map<std::string, std::string> query_;
                std::string fragment_;
                std::map<std::string, std::string> header_;
                std::string content_;

				friend const std::string& ::get(const ::wsf::transport::http::request_t& req,
					const std::string& name,const std::string& default_value);

				friend void ::set(::wsf::transport::http::request_t& req,
					const std::string& name, const std::string& value);

				friend bool ::has(const ::wsf::transport::http::request_t& req, const std::string& name);

				std::map<std::string,std::string> properties__;

            };

            struct response_t
            {
                inline void set_status_code(::wsf::http::status_code code, const std::string &desc = "")
                {
                    status_code_ = static_cast<int>(code);
                    reason_ = desc.empty() ? ::wsf::http::reason(code) : desc;
                }
                inline const int &status_code() const
                {
                    return status_code_;
                }
                inline int &status_code()
                {
                    return status_code_;
                }

                inline std::string &reason()
                {
                    return reason_;
                }
                inline const std::string &reason() const
                {
                    return reason_;
                }

                inline std::map<std::string, std::string> &header()
                {
                    return header_;
                }
                inline const std::map<std::string, std::string> &header() const
                {
                    return header_;
                }

                inline std::string &content()
                {
                    return content_;
                }

                inline const std::string &content() const
                {
                    return content_;
                }


            protected:
                int status_code_;
                std::string reason_;
                std::map<std::string, std::string> header_;
                std::string content_;

				// property used attach user data
				friend const std::string& ::get(const ::wsf::transport::http::response_t& res,
					const std::string& name, const std::string& default_value);

				friend void ::set(::wsf::transport::http::response_t& res,
					const std::string& name, const std::string& value);

				friend bool ::has(const ::wsf::transport::http::response_t& res, const std::string& name);

				std::map<std::string, std::string> properties__;
			};


            class Response
            {
            public:
				Response(context_ptr& context)
					: context_(context.release())
					, response_( new response_t)
				{
					set_status_code(::wsf::http::status_code::OK);
					response_->header()["Content-Type"] = "application/json";
				}

				Response(context_ptr& context,response_ptr& res )
					: context_(context.release())
 					, response_( res.release())
 				{
 					set_status_code(::wsf::http::status_code::OK);
 					response_->header()["Content-Type"] = "application/json";
 				}
                
                //property
                inline void set_status_code(::wsf::http::status_code code, const std::string &desc = "")
                {
                    response_->set_status_code(code, desc);
                }
                inline const int &status_code() const
                {
                    return response_->status_code();
                }
                inline int &status_code()
                {
                    return response_->status_code();
                }
                inline std::string &reason()
                {
                    return response_->reason();
                }
                inline const std::string &reason() const
                {
                    return response_->reason();
                }
                inline std::map<std::string, std::string> &header()
                {
                    return response_->header();
                }
                inline const std::map<std::string, std::string> &header() const
                {
                    return response_->header();
                }

                inline std::string &content()
                {
                    return response_->content();
                }

                inline const std::string &content() const
                {
                    return response_->content();
                }

                //method
                virtual void Reply()
                {
                    ResponseToRemote(context_, response_);
                }

            protected:
                context_ptr  context_;
                response_ptr response_;
            };
        }
    }
}

inline const std::string& get(const ::wsf::transport::http::request_t& req,
	const std::string& name,const std::string& default_value)
{
	std::map<std::string,std::string>::const_iterator it = req.properties__.find(name);
	return it == req.properties__.cend() ? default_value : it->second;
}

inline int get(const ::wsf::transport::http::request_t& req,
	const std::string& name, const int default_value)
{
	const std::string& value = get(req, name);
	return value.empty() ? default_value : std::stoi(value);
}

inline void set(::wsf::transport::http::request_t& req, 
	const std::string& name, const std::string& value)
{
	req.properties__[name] = value;
}



inline bool has(const ::wsf::transport::http::request_t& req, const std::string& name)
{
	return req.properties__.cend() != req.properties__.find(name);
}


inline const std::string& get(const ::wsf::transport::http::response_t& res,
	const std::string& name, const std::string& default_value)
{
	std::map<std::string, std::string>::const_iterator it = res.properties__.find(name);
	return it == res.properties__.cend() ? default_value : it->second;
}


inline void set(::wsf::transport::http::response_t& res,
	const std::string& name, const std::string& value)
{
	res.properties__[name] = value;
}

inline bool has(const ::wsf::transport::http::response_t& res, const std::string& name)
{
	return res.properties__.cend() != res.properties__.find(name);
}
#endif