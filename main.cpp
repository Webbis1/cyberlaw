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
#include <sys/stat.h>    // <--- добавьте эту строку

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

// --- Вставка поста в wp_posts (WordPress) ---
void insert_wp_post(MYSQL *conn, const string &title, const string &content)
{
    // Значения по умолчанию для новых постов
    string post_status = "draft";
    string post_type = "post";
    int post_author = 1; // ID автора (можно изменить)
    string post_excerpt = "";
    string post_name = ""; // можно сгенерировать из title
    string guid = "";

    // Текущее время для post_date и post_date_gmt
    time_t now = time(nullptr);
    struct tm *ltm = gmtime(&now);
    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", ltm);
    string post_date = date_buf;
    string post_date_gmt = date_buf;

    string esc_title = sql_escape(conn, title);
    string esc_content = sql_escape(conn, content);

    // Можно сгенерировать post_name (slug) из title (упрощённо)
    string slug = esc_title;
    for (auto &c : slug) {
        if (c == ' ') c = '-';
        else if (!isalnum(c) && c != '-') c = '\0';
    }
    slug.erase(remove(slug.begin(), slug.end(), '\0'), slug.end());

    // guid обычно формируется как http://site/?p=<id>, но можно оставить пустым (WordPress сам обновит)
    string q =
        "INSERT INTO wp_posts "
        "(post_author, post_date, post_date_gmt, post_content, post_title, post_excerpt, post_status, comment_status, ping_status, post_name, post_type) "
        "VALUES (" +
        to_string(post_author) + ", '" + post_date + "', '" + post_date_gmt + "', '" + esc_content + "', '" + esc_title +
        "', '" + post_excerpt + "', '" + post_status + "', 'open', 'open', '" + slug + "', '" + post_type + "')";

    if (mysql_query(conn, q.c_str())) {
        cerr << get_current_time() << " ERROR: INSERT wp_posts failed: " << mysql_error(conn) << endl;
    } else {
        cout << get_current_time() << " New WordPress post inserted: " << esc_title << endl;
    }
}

// --- Реализация функции find_article_links ---
vector<string> find_article_links(HtmlParser &parser, const string &pattern)
{
    vector<string> links = parser.find_by_selector("a");
    vector<string> filtered_links;

    regex re(pattern);
    for (const string &link : links)
    {
        if (regex_search(link, re))
        {
            filtered_links.push_back(link);
        }
    }

    return filtered_links;
}

// --- Реализация функции process_article ---
string process_article(const string &base_url, const string &article_url, const string &content_pattern)
{
    string html = download_html(article_url);
    HtmlParser parser(html);

    // Используем более надежный способ получения контента статьи
    vector<string> content_blocks = parser.find_by_class(content_pattern);
    if (content_blocks.empty())
    {
        cerr << get_current_time() << " ERROR: No content found for article: " << article_url << endl;
        return "";
    }

    // Объединяем все найденные блоки контента в один
    stringstream ss;
    for (const string &block : content_blocks)
    {
        ss << block << "\n";
    }

    return ss.str();
}

// --- Реализация функции save_results ---
void save_results(const string &output_dir, const string &site_host, const vector<pair<string, string>> &articles)
{
    for (const auto &[url, content] : articles)
    {
        // Генерируем имя файла на основе URL
        string file_name = url;
        replace(file_name.begin(), file_name.end(), '/', '_');
        file_name = output_dir + "/" + file_name + ".html";

        ofstream ofs(file_name);
        if (ofs)
        {
            ofs << content;
            cout << get_current_time() << " Article saved: " << file_name << endl;
        }
        else
        {
            cerr << get_current_time() << " ERROR: Failed to save article: " << file_name << endl;
        }
    }
}

// --- Реализация функции read_config ---
ParserConfig read_config(const string &config_file)
{
    ParserConfig config;
    ifstream ifs(config_file);
    if (!ifs)
    {
        throw runtime_error("Failed to open config file: " + config_file);
    }

    Json::Value root;
    ifs >> root;

    // Чтение конфигурации сайтов
    for (const auto &site : root["sites"])
    {
        SiteConfig site_config;
        site_config.url = site["url"].asString();
        site_config.link_pattern = site["link_pattern"].asString();
        site_config.content_block = site["content_block"].asString();
        site_config.max_pages = site["max_pages"].asInt();

        config.sites.push_back(site_config);
    }

    // Чтение общей конфигурации парсера
    config.output_dir = root["output_dir"].asString();
    config.request_delay = root["request_delay"].asInt();

    // Чтение конфигурации базы данных
    const auto &db = root["db"];
    config.db.host = db["host"].asString();
    config.db.user = db["user"].asString();
    config.db.password = db["password"].asString();
    config.db.database = db["database"].asString();
    config.db.port = db["port"].asUInt();

    return config;
}

// --- Реализация функции ensure_dir_exists ---
void ensure_dir_exists(const string &dir)
{
    struct stat info;

    if (stat(dir.c_str(), &info) != 0)
    {
        cerr << get_current_time() << " ERROR: Cannot access " << dir << endl;
        return;
    }

    if (info.st_mode & S_IFDIR)
    {
        cout << get_current_time() << " Directory exists: " << dir << endl;
    }
    else
    {
        cerr << get_current_time() << " ERROR: " << dir << " is not a directory" << endl;
    }
}

// --- Прототипы функций ---
string process_article(const string &base_url, const string &article_url, const string &content_pattern);
void save_results(const string &output_dir, const string &site_host, const vector<pair<string, string>> &articles);
ParserConfig read_config(const string &config_file);
void ensure_dir_exists(const string &dir);
void process_site(const SiteConfig &site, const ParserConfig &config, MYSQL *conn); // <--- добавьте этот прототип

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

            // --- Вставка в WordPress ---
            string post_title = article_url;
            insert_wp_post(conn, post_title, content);
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

// Подключение к БД использует параметры из config.db, host берётся из config.json