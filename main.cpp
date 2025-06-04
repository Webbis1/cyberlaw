#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <cstring>
#include <cstdlib>
#include <jsoncpp/json/json.h>
#include <regex>
#include <unistd.h>
#include <iomanip>
#include <mysql/mysql.h> // MariaDB/MySQL C API

using std::regex;
using std::smatch;
using std::sregex_iterator;

using namespace std;

// Функция для получения текущего времени в формате [HH:MM:SS]
string get_current_time()
{
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    stringstream ss;
    ss << "[" << setw(2) << setfill('0') << ltm->tm_hour << ":"
       << setw(2) << setfill('0') << ltm->tm_min << ":"
       << setw(2) << setfill('0') << ltm->tm_sec << "]";
    return ss.str();
}

class HtmlParser
{
private:
    string html_content;
    htmlDocPtr doc;
    friend vector<string> find_article_links(HtmlParser &, const string &);

public:
    HtmlParser(const string &html) : html_content(html), doc(nullptr)
    {
        doc = htmlReadDoc((const xmlChar *)html_content.c_str(),
                          NULL, NULL,
                          HTML_PARSE_RECOVER |
                              HTML_PARSE_NOERROR |
                              HTML_PARSE_NOWARNING);

        // Добавьте проверку
        if (!doc)
        {
            cerr << "Failed to parse HTML document" << endl;
            ofstream bad_html("bad_html.html");
            bad_html << html_content;
            bad_html.close();
        }
    }
    ~HtmlParser()
    {
        if (doc)
        {
            xmlFreeDoc(doc);
            cout << get_current_time() << " HTML parser resources freed" << endl;
        }
    }

    bool is_valid() const
    {
        return doc != nullptr;
    }

    vector<string> find_by_class(const string &class_name)
    {
        vector<string> results;
        if (!doc)
        {
            cerr << get_current_time() << " ERROR: No valid document for class search" << endl;
            return results;
        }

        cout << get_current_time() << " Searching for elements with class: " << class_name << endl;
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context)
        {
            cerr << get_current_time() << " ERROR: Failed to create XPath context" << endl;
            return results;
        }

        string xpath = "//*[contains(@class, '" + class_name + "')]";
        xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *)xpath.c_str(), context);

        if (result && result->nodesetval)
        {
            cout << get_current_time() << " Found " << result->nodesetval->nodeNr << " elements" << endl;
            for (int i = 0; i < result->nodesetval->nodeNr; ++i)
            {
                xmlNodePtr node = result->nodesetval->nodeTab[i];
                xmlChar *content = xmlNodeGetContent(node);
                if (content)
                {
                    results.push_back(string((char *)content));
                    xmlFree(content);
                }
            }
        }
        else
        {
            cout << get_current_time() << " No elements found with class: " << class_name << endl;
        }

        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        return results;
    }

    vector<string> find_by_selector(const string &selector)
    {
        vector<string> results;
        if (!doc)
        {
            cerr << get_current_time() << " ERROR: No valid document for selector search" << endl;
            return results;
        }

        cout << get_current_time() << " Searching with selector: " << selector << endl;
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context)
        {
            cerr << get_current_time() << " ERROR: Failed to create XPath context" << endl;
            return results;
        }

        xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *)selector.c_str(), context);

        if (result && result->nodesetval)
        {
            cout << get_current_time() << " Found " << result->nodesetval->nodeNr << " elements" << endl;
            for (int i = 0; i < result->nodesetval->nodeNr; ++i)
            {
                xmlNodePtr node = result->nodesetval->nodeTab[i];
                xmlChar *content = xmlNodeGetContent(node);
                if (content)
                {
                    results.push_back(string((char *)content));
                    xmlFree(content);
                }
            }
        }
        else
        {
            cout << get_current_time() << " No elements found with selector: " << selector << endl;
        }

        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        return results;
    }

    vector<string> find_by_regex(const string &pattern)
    {
        vector<string> results;
        if (!doc)
        {
            cerr << get_current_time() << " ERROR: No valid document for regex search" << endl;
            return results;
        }

        cout << get_current_time() << " Searching with regex pattern: " << pattern << endl;
        regex re(pattern);
        smatch matches;
        string content = html_content;

        sregex_iterator it(content.begin(), content.end(), re);
        sregex_iterator end;

        int count = 0;
        for (; it != end; ++it)
        {
            results.push_back(it->str());
            count++;
        }

        cout << get_current_time() << " Found " << count << " regex matches" << endl;
        return results;
    }

    string get_element_content(const string &xpath)
    {
        if (!doc)
            return "";

        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context)
            return "";

        // Добавим проверку выражения
        if (xpath.empty())
        {
            cerr << "XPath expression is empty!" << endl;
            return "";
        }

        cout << "Executing XPath: " << xpath << endl;

        xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *)xpath.c_str(), context);
        string content;

        if (result)
        {
            if (result->nodesetval && result->nodesetval->nodeNr > 0)
            {
                cout << "Found " << result->nodesetval->nodeNr << " nodes" << endl;
                xmlNodePtr node = result->nodesetval->nodeTab[0];
                xmlChar *text = xmlNodeGetContent(node);
                if (text)
                {
                    content = string((char *)text);
                    xmlFree(text);

                    // Отладочный вывод первых 100 символов
                    cout << "Content preview: "
                         << content.substr(0, min(100, (int)content.size()))
                         << (content.size() > 100 ? "..." : "")
                         << endl;
                }
            }
            else
            {
                cerr << "No nodes found for XPath" << endl;
            }
            xmlXPathFreeObject(result);
        }
        else
        {
            cerr << "XPath evaluation failed" << endl;
        }

        xmlXPathFreeContext(context);
        return content;
    }
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *output)
{
    size_t total_size = size * nmemb;
    output->append((char *)contents, total_size);
    return total_size;
}

string download_html(const string &url)
{
    cout << get_current_time() << " Downloading URL: " << url << endl;
    CURL *curl = curl_easy_init();
    string html;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << get_current_time() << " CURL ERROR: " << curl_easy_strerror(res) << endl;
        }
        else
        {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            cout << get_current_time() << " HTTP status: " << http_code << ", downloaded " << html.size() << " bytes" << endl;
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        cerr << get_current_time() << " ERROR: Failed to initialize CURL" << endl;
    }

    return html;
}

struct SiteConfig
{
    string url;
    string link_pattern;
    string content_block;
    int max_pages = 10;
};

struct DbConfig {
    string host = "localhost";
    string user = "root";
    string password = "";
    string database = "cyberlaw";
    unsigned int port = 3306;
};

struct ParserConfig
{
    vector<SiteConfig> sites;
    string output_dir = "output";
    int request_delay = 1; // уменьшено до 1 секунды для тестов
    DbConfig db;
    string post_url = "";
};

// --- Прототип функции экранирования строки для SQL ---
string sql_escape(MYSQL *conn, const string &str);

// --- MariaDB helpers ---
MYSQL *init_db(const DbConfig &cfg)
{
    MYSQL *conn = mysql_init(nullptr);
    if (!conn) {
        cerr << get_current_time() << " ERROR: mysql_init failed" << endl;
        exit(1);
    }
    // Используем TCP/IP соединение, а не сокет
    if (!mysql_real_connect(conn, cfg.host.c_str(), cfg.user.c_str(), cfg.password.c_str(),
                            cfg.database.c_str(), cfg.port, NULL, 0)) {
        cerr << get_current_time() << " ERROR: mysql_real_connect failed: " << mysql_error(conn) << endl;
        mysql_close(conn);
        exit(1);
    }
    // Создать таблицу если не существует
    const char *create_sql = "CREATE TABLE IF NOT EXISTS articles (url VARCHAR(191) PRIMARY KEY)";
    if (mysql_query(conn, create_sql)) {
        cerr << get_current_time() << " ERROR: create table failed: " << mysql_error(conn) << endl;
        mysql_close(conn);
        exit(1);
    }
    return conn;
}

bool url_in_db(MYSQL *conn, const string &url)
{
    string esc_url = sql_escape(conn, url);
    string q = "SELECT url FROM articles WHERE url='" + esc_url + "'";
    cout << get_current_time() << " [DEBUG] Checking URL in DB (" << url.length() << "): " << url << endl;
    cout << get_current_time() << " [DEBUG] SQL: " << q << endl;
    if (mysql_query(conn, q.c_str())) {
        cerr << get_current_time() << " ERROR: SELECT failed: " << mysql_error(conn) << endl;
        return false;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    bool found = (res && mysql_num_rows(res) > 0);
    if (res) mysql_free_result(res);
    return found;
}

void insert_url(MYSQL *conn, const string &url)
{
    string esc_url = sql_escape(conn, url);
    string q = "INSERT IGNORE INTO articles(url) VALUES('" + esc_url + "')";
    cout << get_current_time() << " [DEBUG] Inserting URL in DB (" << url.length() << "): " << url << endl;
    cout << get_current_time() << " [DEBUG] SQL: " << q << endl;
    if (mysql_query(conn, q.c_str())) {
        cerr << get_current_time() << " ERROR: INSERT failed: " << mysql_error(conn) << endl;
    }
}

// --- Экранирование строки для SQL ---
string sql_escape(MYSQL *conn, const string &str)
{
    size_t len = str.size();
    char *buf = new char[len * 2 + 1];
    unsigned long out_len = mysql_real_escape_string(conn, buf, str.c_str(), len);
    string out(buf, out_len);
    delete[] buf;
    return out;
}

// --- POST запрос ---
bool send_post(const string &url, const string &article_url, const string &content)
{
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    Json::Value root;
    root["url"] = article_url;
    root["content"] = content;
    string json = root.toStyledString();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    CURLcode res = curl_easy_perform(curl);
    bool ok = (res == CURLE_OK);
    if (!ok) {
        cerr << get_current_time() << " POST ERROR: " << curl_easy_strerror(res) << endl;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ok;
}

// --- Чтение конфига с поддержкой db и post_url ---
ParserConfig read_config(const string &config_file)
{
    cout << get_current_time() << " Reading config file: " << config_file << endl;
    ParserConfig config;

    ifstream file(config_file);
    if (!file.is_open())
    {
        cerr << get_current_time() << " ERROR: Cannot open config file: " << config_file << endl;
        exit(1);
    }

    Json::Value root;
    file >> root;

    for (const auto &site : root["sites"])
    {
        SiteConfig sc;
        sc.url = site["url"].asString();
        sc.link_pattern = site["link_pattern"].asString();
        sc.content_block = site["content_block"].asString();
        if (site.isMember("max_pages"))
        {
            sc.max_pages = site["max_pages"].asInt();
        }
        cout << get_current_time() << " Configured site: " << sc.url
             << ", max pages: " << sc.max_pages << endl;
        config.sites.push_back(sc);
    }

    if (root.isMember("output_dir"))
    {
        config.output_dir = root["output_dir"].asString();
    }

    if (root.isMember("request_delay"))
    {
        config.request_delay = root["request_delay"].asInt();
    }
    if (root.isMember("db")) {
        const auto &db = root["db"];
        if (db.isMember("host")) config.db.host = db["host"].asString();
        if (db.isMember("user")) config.db.user = db["user"].asString();
        if (db.isMember("password")) config.db.password = db["password"].asString();
        if (db.isMember("database")) config.db.database = db["database"].asString();
        if (db.isMember("port")) config.db.port = db["port"].asUInt();
    }
    if (root.isMember("post_url")) {
        config.post_url = root["post_url"].asString();
    }

    cout << get_current_time() << " Config loaded successfully. Output directory: "
         << config.output_dir << ", request delay: " << config.request_delay << "s" << endl;

    return config;
}

vector<string> find_article_links(HtmlParser &parser, const string &pattern)
{
    vector<string> links;
    if (!parser.doc)
    {
        cerr << get_current_time() << " ERROR: No valid document for link search" << endl;
        return links;
    }

    cout << get_current_time() << " Searching for article links with pattern: " << pattern << endl;
    xmlXPathContextPtr context = xmlXPathNewContext(parser.doc);
    if (!context)
    {
        cerr << get_current_time() << " ERROR: Failed to create XPath context" << endl;
        return links;
    }

    // Изменим XPath-запрос - убираем /@href в конце
    string modified_pattern = pattern;
    if (pattern.find("/@href") != string::npos)
    {
        modified_pattern = pattern.substr(0, pattern.find("/@href"));
        cout << get_current_time() << " Modified pattern to: " << modified_pattern << endl;
    }

    xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *)modified_pattern.c_str(), context);

    if (result && result->nodesetval)
    {
        cout << get_current_time() << " Found " << result->nodesetval->nodeNr << " nodes matching pattern" << endl;

        for (int i = 0; i < result->nodesetval->nodeNr; ++i)
        {
            xmlNodePtr node = result->nodesetval->nodeTab[i];
            cout << get_current_time() << " Processing node #" << i + 1 << " of type " << node->type
                 << ", name: " << node->name << endl;

            // Для элемента <a> получаем атрибут href
            if (node->type == XML_ELEMENT_NODE && strcmp((char *)node->name, "a") == 0)
            {
                xmlChar *href = xmlGetProp(node, (const xmlChar *)"href");
                if (href)
                {
                    string link = string((char *)href);
                    cout << get_current_time() << " Found valid link: " << link << endl;
                    links.push_back(link);
                    xmlFree(href);
                }
                else
                {
                    cerr << get_current_time() << " WARNING: No href found in <a> element" << endl;
                }
            }
        }
    }
    else
    {
        cout << get_current_time() << " No nodes found with given pattern" << endl;
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);

    cout << get_current_time() << " Total links found: " << links.size() << endl;
    return links;
}

string process_article(const string &base_url, const string &article_url, const string &content_pattern)
{
    // Если ссылка уже абсолютная - используем как есть
    string full_url = article_url;

    // Если относительная - добавляем базовый URL
    if (article_url.find("http") != 0)
    {
        // Убедимся, что base_url заканчивается на /, а article_url не начинается с /
        string base = base_url;
        if (base.back() != '/')
            base += '/';
        if (!article_url.empty() && article_url.front() == '/')
        {
            full_url = base + article_url.substr(1);
        }
        else
        {
            full_url = base + article_url;
        }
    }

    cout << get_current_time() << " Final article URL: " << full_url << endl;
    string html = download_html(full_url);
    HtmlParser parser(html);
    if (!parser.is_valid())
    {
        cerr << get_current_time() << " ERROR: Failed to parse article: " << full_url << endl;
        return "";
    }

    string content = parser.get_element_content(content_pattern);
    cout << get_current_time() << " Extracted " << content.size() << " characters from article" << endl;
    return content;
}

void ensure_dir_exists(const string &dir)
{
    cout << get_current_time() << " Checking/Creating directory: " << dir << endl;
    string command = "mkdir -p " + dir;
    int result = system(command.c_str());
    if (result != 0)
    {
        cerr << get_current_time() << " ERROR: Failed to create directory: " << dir << endl;
    }
    else
    {
        cout << get_current_time() << " Directory ready: " << dir << endl;
    }
}

void save_results(const string &output_dir, const string &site_host, const vector<pair<string, string>> &articles)
{
    ensure_dir_exists(output_dir);

    string filename = output_dir + "/" + site_host + ".json";
    cout << get_current_time() << " Saving results to: " << filename << endl;

    ofstream out(filename);
    if (!out.is_open())
    {
        cerr << get_current_time() << " ERROR: Failed to open output file: " << filename << endl;
        return;
    }

    Json::Value root;
    for (const auto &[url, content] : articles)
    {
        Json::Value article;
        article["url"] = url;
        article["content"] = content;
        root.append(article);
    }

    out << root.toStyledString();
    out.close();
    cout << get_current_time() << " Successfully saved " << articles.size()
         << " articles to " << filename << endl;
}

// --- Основная обработка сайта с БД и POST ---
void process_site(const SiteConfig &site, const ParserConfig &config, MYSQL *conn)
{
    cout << get_current_time() << " ===== Starting to process site: " << site.url << " =====" << endl;

    string main_html = download_html(site.url);
    HtmlParser main_parser(main_html);
    if (!main_parser.is_valid())
    {
        cerr << get_current_time() << " ERROR: Failed to parse main page: " << site.url << endl;
        return;
    }

    vector<string> article_links = find_article_links(main_parser, site.link_pattern);
    cout << get_current_time() << " Total article links found: " << article_links.size() << endl;

    if (article_links.size() > site.max_pages)
    {
        cout << get_current_time() << " Limiting articles from " << article_links.size()
             << " to " << site.max_pages << " (config.max_pages)" << endl;
        article_links.resize(site.max_pages);
    }

    vector<pair<string, string>> articles;
    for (size_t i = 0; i < article_links.size(); ++i)
    {
        const string &article_url = article_links[i];
        // Проверка в БД
        if (url_in_db(conn, article_url)) {
            cout << get_current_time() << " Already in DB, skipping: " << article_url << endl;
            continue;
        }
        // Добавить в БД
        insert_url(conn, article_url);

        cout << get_current_time() << " Processing article " << (i + 1) << "/" << article_links.size() << endl;
        string content = process_article(site.url, article_url, site.content_block);
        if (!content.empty())
        {
            articles.emplace_back(article_url, content);
            // Отправить POST
            if (!config.post_url.empty()) {
                send_post(config.post_url, article_url, content);
            }
        }
        else
        {
            cout << get_current_time() << " WARNING: Empty content for article: " << article_url << endl;
        }

        if (i < article_links.size() - 1)
        {
            cout << get_current_time() << " Waiting " << config.request_delay << " seconds before next request..." << endl;
            sleep(config.request_delay);
        }
    }

    size_t start = site.url.find("://") + 3;
    size_t end = site.url.find("/", start);
    string host = site.url.substr(start, end - start);

    cout << get_current_time() << " ===== Finished processing site: " << site.url << " =====" << endl;
    cout << get_current_time() << " Successfully processed " << articles.size() << "/" << article_links.size() << " articles" << endl;

    save_results(config.output_dir, host, articles);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <config_file.json>" << endl;
        return 1;
    }

    cout << get_current_time() << " Initializing parser..." << endl;
    cout << get_current_time() << " Initializing CURL and libxml2..." << endl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    xmlInitParser();

    try
    {
        ParserConfig config = read_config(argv[1]);
        ensure_dir_exists(config.output_dir);

        // --- MariaDB ---
        MYSQL *conn = init_db(config.db);

        cout << get_current_time() << " Starting to process " << config.sites.size() << " sites" << endl;
        for (size_t i = 0; i < config.sites.size(); ++i)
        {
            cout << get_current_time() << " Processing site " << (i + 1) << "/" << config.sites.size() << endl;
            process_site(config.sites[i], config, conn);

            if (i < config.sites.size() - 1)
            {
                cout << get_current_time() << " Waiting " << config.request_delay << " seconds before next site..." << endl;
                sleep(config.request_delay);
            }
        }
        mysql_close(conn);
        cout << get_current_time() << " All sites processed successfully" << endl;
    }
    catch (const exception &e)
    {
        cerr << get_current_time() << " EXCEPTION: " << e.what() << endl;
    }

    cout << get_current_time() << " Cleaning up resources..." << endl;
    xmlCleanupParser();
    curl_global_cleanup();
    cout << get_current_time() << " Parser finished work" << endl;

    return 0;
}