#include <wsf/transport/httpws.hpp>
#include <wsf/util/logger.hpp>
#include <wsf/util/uriparser.hpp>
#include <http/helper.hpp>
#include <vector>
#include <mutex>

namespace wsf
{
    namespace transport
    {
        namespace httpws
        {
            LOGGER("wsf.transport.httpws");

            namespace
            {
                std::mutex server_seq_mutex;
                std::map<std::string, response_fn> server_request_list;
                std::mutex client_seq_mutex;
                std::map<std::string, response_fn> client_request_list;
                inline static unsigned long serial_number()
                {
#ifdef WIN32
                    static unsigned long serial_number = 0;
                    return ::InterlockedIncrement(&serial_number);
#else

                    //TODO
                    static unsigned long serial_number = 0;
                    return ::__sync_add_and_fetch(&serial_number, 1);
#endif
                }

                void SplitString(const std::string &s, std::vector<std::string> &v, const std::string &c)
                {
                    std::string::size_type pos1, pos2;
                    pos2 = s.find(c);
                    pos1 = 0;
                    while (std::string::npos != pos2)
                    {
                        v.push_back(s.substr(pos1, pos2 - pos1));

                        pos1 = pos2 + c.size();
                        pos2 = s.find(c, pos1);
                    }
                    if (pos1 != s.length())
                    {
                        v.push_back(s.substr(pos1));
                    }
                }

                bool ParseRequest(request_t *req,
                                  const std::vector<std::string> &first_line,
                                  const std::string &header,
                                  const std::string &body)
                {
                    req->method() = first_line[0];
                    //decode url
                    size_t *size_out = NULL;
                    char *decode_uri = evhttp_uridecode(first_line[1].c_str(), 0, size_out);
                    if (decode_uri == NULL)
                    {
                        free(decode_uri);
                        return false;
                    }
                    //parse url
                    struct evhttp_uri *ev_decode_url = evhttp_uri_parse_with_flags(decode_uri, EVHTTP_URI_NONCONFORMANT);
                    if (ev_decode_url == NULL)
                    {
                        free(decode_uri);
                        evhttp_uri_free(ev_decode_url);
                        return false;
                    }
                    //->path
                    const char *path = evhttp_uri_get_path(ev_decode_url);
                    if (path == NULL)
                    {
                        free(decode_uri);
                        evhttp_uri_free(ev_decode_url);
                        return false;
                    }
                    const char *query = evhttp_uri_get_query(ev_decode_url);
                    const char *fragment = evhttp_uri_get_fragment(ev_decode_url);
                    req->path().assign(path);
                    //->query
                    if (query != NULL)
                    {
                        struct evkeyvalq ev_query;
                        evhttp_parse_query_str(query, &ev_query);
                        struct evkeyval *key_val;
                        WSF_TAILQ_FOREACH(key_val, &ev_query, next)
                        {
                            std::string key(key_val->key);
                            std::string value(key_val->value);
                            req->query()[key] = value;
                        }
					}
                    //->fragment
                    if (fragment != NULL)
                    {
                        req->fragment().assign(fragment);
                    }
                    //->header
                    if (!header.empty())
                    {
                        std::vector<std::string> split_headers;
                        SplitString(header, split_headers, "\r\n");
                        size_t pos;
                        for (auto &it : split_headers)
                        {
                            pos = it.find(":");
                            if (pos == std::string::npos)
                            {
                                continue;
                            }
                            req->header()[it.substr(0, pos)] = it.substr(pos + 1);
                        }
                    }
                    //->body
                    if (!body.empty())
                    {
                        req->content().assign(body.c_str(), body.size());
                    }

                    free(decode_uri);
                    evhttp_uri_free(ev_decode_url);
                    return true;
                }

                bool ParseResponse(response_t *res,
                                   const std::vector<std::string> &first_line,
                                   const std::string &header,
                                   const std::string &body)
                {
                    res->set_status_code((::wsf::http::status_code)std::stoi(first_line[1]));
                    //->header
                    if (!header.empty())
                    {
                        std::vector<std::string> split_headers;
                        SplitString(header, split_headers, "\r\n");
                        size_t pos;
                        for (auto &it : split_headers)
                        {
                            pos = it.find(":");
                            if (pos == std::string::npos)
                            {
                                continue;
                            }
                            res->header()[it.substr(0, pos)] = it.substr(pos + 1);
                        }
                    }
                    //->body
                    if (!body.empty())
                    {
                        res->content().assign(body.c_str(), body.size());
                    }

                    return true;
                }
                bool SerializeResponse(std::string &data, response_t *res)
                {
                    if (res->status_code() < 100 ||
                        res->status_code() > 599 ||
                        res->reason().empty())
                    {
                        return false;
                    }
                    data.clear();
                    data.append("HTTP/1.1 ").append(std::to_string(res->status_code()));
                    data.append(" ").append(res->reason()).append("\r\n");
                    for (auto &it : res->header())
                    {
                        data.append(it.first).append(":").append(it.second).append("\r\n");
                    }
                    data.append("\r\n");
                    data.append(res->content());
                    return true;
                }
                bool SerializeRequest(std::string &data, request_t *req)
                {
                    if (req->method().empty() ||
                        req->path().empty())
                    {
                        return false;
                    }
                    data.clear();
                    data.append(req->method()).append(" ");
                    std::string url = req->path();
                    if (!req->query().empty())
                    {
                        auto it = req->query().begin();
                        url.append("?").append(it->first).append("=").append(it->second);
                        for (; it != req->query().end(); ++it)
                        {
                            url.append("&").append(it->first).append("=").append(it->second);
                        }
                    }
                    if (!req->fragment().empty())
                    {
                        url.append("#").append(req->fragment());
                    }
                    data.append(url).append(" HTTP/1.1\r\n");

                    for (auto &it : req->header())
                    {
                        data.append(it.first).append(":").append(it.second).append("\r\n");
                    }
                    data.append("\r\n");
                    data.append(req->content());
                    return true;
                }
                bool Ws2Http(const std::string &data, request_t *&req, response_t *&res)
                {
                    size_t offset_header = data.find("\r\n");
                    size_t offset_body = data.find("\r\n\r\n");
                    if (offset_header == std::string::npos ||
                        offset_body == std::string::npos)
                    {
                        return false;
                    }
                    std::vector<std::string> first_line;
                    SplitString(data.substr(0, offset_header), first_line, " ");
                    if (first_line.size() < 3)
                    {
                        return false;
                    }
                    //request
                    if (first_line[2].find("HTTP/") != std::string::npos)
                    {
                        std::string header = data.substr(offset_header + 2, offset_body - offset_header);
                        std::string body = data.substr(offset_body + 4);
                        req = new request_t;
                        bool rc = ParseRequest(req, first_line, header, body);

                        return rc;
                    }
                    //response
                    if (first_line[0].find("HTTP/") != std::string::npos)
                    {
                        std::string header = data.substr(offset_header + 2, offset_body - offset_header);
                        std::string body = data.substr(offset_body + 4);
                        res = new response_t;
                        bool rc = ParseResponse(res, first_line, header, body);

                        return rc;
                    }
                    return false;
                }
            }
            bool Bind(const std::string &url, request_fn fn)
            {

                auto callback = [url, fn](::wsf::transport::websocket::msg_status status,
                                          const std::string &peer_token,
                                          const std::string &data) {
                    request_t *req = NULL;
                    response_t *res = NULL;
                    if (status == ::wsf::transport::websocket::msg_status::OPEN ||
                        status == ::wsf::transport::websocket::msg_status::CLOSED)
                    {
                        context_ptr context(new common_context_t(peer_token, WS_CLIENT, status));
                        request_ptr request(req);
                        fn(context, request);
                        return;
                    }
                    if (!Ws2Http(url, req, res))
                    {
                        return;
                    }
                    if (req != NULL)
                    {
                        context_ptr context(new common_context_t(peer_token, WS_SERVER, status, req->header()["CSeq"]));
                        request_ptr request(req);
                        fn(context, request);
                    }
                    if (res != NULL)
                    {
                        context_ptr context(new common_context_t(peer_token, WS_SERVER, status));
                        response_ptr response(res);

                        if (res->header().find("CSeq") == res->header().end())
                        {
                            ERR(" Response without CSeq!");
                            return;
                        }
                        server_seq_mutex.lock();
                        std::string seq = res->header()["CSeq"];
                        char *p = const_cast<char *>(seq.c_str());
                        while (*p == 32)
                        {
                            ++p;
                        }
                        response_fn fn = server_request_list[std::string(p)];
                        if (fn != NULL)
                        {
                            fn(context, response);
                        }
                        else
                        {
                            ERR("Cant't find the response callback with CSeq: " << std::string(p));
                        }
                        server_request_list.erase(seq);
                        server_seq_mutex.unlock();
                    }
                };
                bool rc = websocket::Bind(url, callback);
                return rc;
            }
            void ResponseToRemote(context_ptr &context, response_ptr &res)
            {
                common_context_t *c = static_cast<common_context_t *>(context.get());

                std::string data;
                res->header()["CSeq"] = c->seq.empty() ? "0" : c->seq;
                if (SerializeResponse(data, res.get()))
                {
                    if (c->content_role == WS_SERVER)
                    {
                        std::list<std::string> list_peers;
                        list_peers.push_back(c->peer_token);

                        websocket::PushMsg(list_peers, data);
                    }
                    else
                    {
                        websocket::SendMsg(c->peer_token, data);
                    }
                }
                else
                {
                    ERR("Failed to seralize response!");
                }
            }

            bool Connect(const std::string &url, request_fn fn)
            {
                ::wsf::util::URIParser parser;
                if (!parser.Parse(url))
                {
                    ERR(" Failed to parse topic url: " << url);
                    return false;
                }
                uint16_t port = parser.port();
                std::string host = parser.host();
                std::string schema = parser.schema();

                std::string peer_base_url = schema + "://" + host + ":" + std::to_string(port);

                auto callback = [url, fn, peer_base_url](::wsf::transport::websocket::msg_status status,
                                                         const std::string &data) {
                    request_t *req = NULL;
                    response_t *res = NULL;
                    if (status == ::wsf::transport::websocket::msg_status::OPEN ||
                        status == ::wsf::transport::websocket::msg_status::CLOSED)
                    {
                        context_ptr context(new common_context_t(peer_base_url, WS_CLIENT, status));
                        request_ptr request(req);
                        fn(context, request);
                        return;
                    }
                    if (!Ws2Http(data, req, res))
                    {
                        return;
                    }
                    if (req != NULL)
                    {
                        context_ptr context(new common_context_t(peer_base_url, WS_CLIENT, status, req->header()["CSeq"]));
                        request_ptr request(req);
                        fn(context, request);
                    }
                    if (res != NULL)
                    {
                        context_ptr context(new common_context_t(peer_base_url, WS_CLIENT, status));
                        response_ptr response(res);

                        if (res->header().find("CSeq") == res->header().end())
                        {
                            ERR(" Response without CSeq!");
                            return;
                        }
                        client_seq_mutex.lock();
                        std::string seq = res->header()["CSeq"];
                        char *p = const_cast<char *>(seq.c_str());
                        while (*p == 32)
                        {
                            ++p;
                        }
                        response_fn callback = client_request_list[std::string(p)];
                        client_request_list.erase(seq);
                        client_seq_mutex.unlock();

                        if (callback != NULL)
                        {
                            callback(context, response);
                        }
                        else
                        {
                            ERR("Cant't find the response callback with CSeq: " << std::string(p));
                        }
                    }
                };
                bool rc = websocket::Connect(url, callback);
                return rc;
            }
            void DisConnect(const std::string &url)
            {
                websocket::DisConnect(url);
            }
            void RequestToRemote(context_ptr &context, request_ptr &req, response_fn fn)
            {
                common_context_t *c = static_cast<common_context_t *>(context.get());

                std::string data;
                std::string seq = std::to_string(serial_number());
                req->header()["CSeq"] = seq;
                req->header()["Content-Type"] = "application/octet-stream";
                req->header()["Content-Length"] = std::to_string(req->content().size());

                if (SerializeRequest(data, req.get()))
                {
                    if (c->content_role == WS_SERVER)
                    {
                        std::list<std::string> list_peers;
                        list_peers.push_back(c->peer_token);

                        server_seq_mutex.lock();
                        server_request_list[seq] = fn;
                        server_seq_mutex.unlock();

                        websocket::PushMsg(list_peers, data);
                    }
                    else
                    {
                        client_seq_mutex.lock();
                        client_request_list[seq] = fn;
                        client_seq_mutex.unlock();

                        websocket::SendMsg(c->peer_token, data);
                    }
                }
                else
                {
                    ERR("Failed to SerializeRequest");
                }
            }
        }
    }
}