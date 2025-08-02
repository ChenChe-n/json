#include "cc_json.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

int main()
{
    using cc_json = chenc::cc_json;
    using js = cc_json;
    cc_json json = {
        {"id", {true, 2, 99.57}},
        {"age", {{"张三", 18}, {"李四", 19}, {"王五", {{"ID", "21814"}, {"name", "王五"}, {"age", 20}, {"city", "上海"}}}, {"", js::nul{}}}},
    };
    std::cout << "json1--------------------" << std::endl;
    std::cout << json.to_string() << std::endl;
    std::cout << json.type_str() << std::endl;
    std::cout << json["age"]["王五"].type_str() << std::endl;

    // 访问数据
    std::cout << "ID[0]: " << (json["id"][0].get_boolean().get_value()) << std::endl;
    std::cout << "张三的年龄: " << (json["age"]["张三"].get_number().get_string()) << std::endl;
    std::cout << "王五的年龄: " << (json["age"]["王五"]["city"].get_string().get_string_ref()) << std::endl;

    // // 修改数据
    json["id"][0] = js::num{100};
    json["age"]["李四"] = js::num{20};
    json["city"] = js::str{"Beijing"}; // 添加新字段
    std::cout << "json1.1--------------------" << std::endl;
    std::cout << json.to_string() << std::endl;

    // 访问数据
    std::cout << "ID[0]: " << (json["id"][0].get_number().get_string()) << std::endl;
    std::cout << "张三的年龄: " << (json["age"]["张三"].get_number().get_string()) << std::endl;
    std::cout << "city: " << (json["city"].get_string().get_string_ref()) << std::endl;

    cc_json json2 = cc_json{
        {"name", "Alice"},
        {"age", js::nul{}},
        {"test", js::num{"114514"}},
        {"json1", json.get_object()}, // 嵌套 JSON
    };
    std::cout << "json2--------------------" << std::endl;
    std::cout << json2.to_string() << std::endl;
    json2["json1"]["city"] = "Shanghai";           // 修改嵌套 JSON 的数据
    json2["json1"]["json1"] = {json.get_object()}; //   添加嵌套 JSON

    cc_json json3 = {
        js::str{"hello"},
        js::str{"world"},
        js::num{1},
        js::num{9.2222222222222222222222222e141},
        js::num{1.00000001},
        js::num{std::string{"1.0000000003"}},
        js::num{"1.0000000003"},
        js::bol{true},
        js::nul{},
        {{"name", js::str{"cc"}},
         {"age", js::num{18}}}};
    std::cout << "json3--------------------" << std::endl;
    std::cout << json3.to_string() << std::endl;

    cc_json json4 = js::parse(R"({
      "null_value": null,
      "boolean_true": true,
      "boolean_false": false,
      "integer_number": 123,
      "float_number": 123.4566666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666,
      "scientific_number": 1.23e+10,
      "string_value": "Hello, 世界",
      "escaped_string": "Line1\\nLine2\\tTabbed\\\"Quoted\\\"",
      "empty_array": [],
      "simple_array": [1, "two", true, null],
      "nested_array": [[1, 2], ["a", "b"], [true, false]],
      "empty_object": {},
      "simple_object": {
        "a": 1,
        "b": "text"
      },
      "nested_object": {
        "person": {
          "name": "Alice",
          "age": 30,
          "married": false,
          "children": ["Bob", "Charlie"]
        }
      },
      "deep_nesting": {
        "level1": {
          "level2": {
            "level3": {
              "level4": {
                "value": "deep"
              }
            }
          }
        }
      },
      "unicode_keys": {
        "名字": "张三",
        "年龄": 2.5e+1,
        "城市": "北京"
        }})");
    std::cout << "json4--------------------" << std::endl;
    std::cout << json4.to_string() << std::endl;

    cc_json json5 = js::parse(R"({
      "title": "JSON测试",
      "json.url": "https://abcdefg",
      "keywords": "JSON测试",
      "功能": [
          "JSON美化\n\n\n\\\t",
          "JSON数据类型显示",
          "JSON数组显示角标",
          "高亮显示",
          "错误提示",
          "\u003e",
          "&nbsp;",
          " ",
          "<h1>JSON在线解析</h1>",
          {
              "备注": [
                  "json",
                  "json"
              ]
          }
      ],
      "加入我们": {
          "QQ群": 114514
      },
      "特殊符号": [
          "&currency",
          "&timestamp",
          "&region",
          "&params",
          "&lt;&lt;sane&gt;&gt;",
          "gbk -> utf-8",
          "gbk -> utf-8"
      ],
      "numbers": [
          305667554401374209,
          103248655202358790,
          123456789012345679,
          987654321098765432,
          246813579246813579,
          135792468013579246,
          -1.11111111111111111111e+111,
          -1.11111111111111111111e-111,
          1.11111111111111111111e+111,
          1.11111111111111111111e-111
      ],
      "id2": 22022621134265013,
      "BigNumber": 71357798191653192098,
      "content": "永和九年，岁在癸丑，暮春之初，会于会稽山阴之兰亭，修禊事也。群贤毕至，少长咸集。此地有崇山峻岭，茂林修竹，又有清流激湍，映带左右，引以为流觞曲水，列坐其次。虽无丝竹管弦之盛，一觞一咏，亦足以畅叙幽情。\n是日也，天朗气清，惠风和畅。仰观宇宙之大，俯察品类之盛，所以游目骋怀，足以极视听之娱，信可乐也。\n夫人之相与，俯仰一世。或取诸怀抱，悟言一室之内；或因寄所托，放浪形骸之外。虽趣舍万殊，静躁不同，当其欣于所遇，暂得于己，快然自足，不知老之将至；及其所之既倦，情随事迁，感慨系之矣。向之所欣，俯仰之间，已为陈迹，犹不能不以之兴怀，况修短随化，终期于尽！古人云：“死生亦大矣。”岂不痛哉！\n每览昔人兴感之由，若合一契，未尝不临文嗟悼，不能喻之于怀。固知一死生为虚诞，齐彭殇为妄作。后之视今，亦犹今之视昔，悲夫！故列叙时人，录其所述，虽世殊事异，所以兴怀，其致一也。后之览者，亦将有感于斯文。"
  })");

    std::cout << "json5--------------------" << std::endl;
    std::cout << json5.to_string() << std::endl;

    cc_json json6 = js::parse(R"(
         [
          [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]],
         {"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{}}}}}}}}}}}}}}}}}}}}}}}}}}}
         ]
    )");

    std::cout << "json6--------------------" << std::endl;
    std::cout << json6.to_string() << std::endl;
    std::cout << json6.type_str() << std::endl;

    cc_json json7 = js::parse(R"(
          "test json 7"
    )");

    std::cout << "json7--------------------" << std::endl;
    std::cout << json7.to_string() << std::endl;
    std::cout << json7.type_str() << std::endl;

    // 读取文件
    std::string json_file;
    std::ifstream file("data/test.json");
    // std::ifstream file("data/mergedClasses.json");
    if (file.is_open())
    {
        json_file = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }
    for (int i = 0; i < 3; i++)
    {
        {
            auto start = std::chrono::high_resolution_clock::now();
            cc_json jsonf = js::parse(json_file);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto strf = jsonf.to_string(2, 1, 1);
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "cc_json json_file--------------------" << std::endl;
            std::cout << strf << std::endl;
            std::cout << "hash          : " << std::hash<std::string>()(strf) << std::endl;
            std::cout << "time          : " << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count() << "ms" << std::endl;
            std::cout << "file      byte: " << json_file.size() / 1'000'000.0 << "MB" << std::endl;
            std::cout << "parse     time: " << std::chrono::duration_cast<std::chrono::duration<double>>(t1 - start).count() << "s" << std::endl;
            std::cout << "parse    speed: " << json_file.size() / std::chrono::duration_cast<std::chrono::duration<double>>(t1 - start).count() / 1'000'000.0 << "MB/s" << std::endl;
            std::cout << "to_str    time: " << std::chrono::duration_cast<std::chrono::duration<double>>(end - t1).count() << "s" << std::endl;
            std::cout << "to_str   speed: " << strf.size() / std::chrono::duration_cast<std::chrono::duration<double>>(end - t1).count() / 1'000'000.0 << "MB/s" << std::endl;
        }
    }
    return 0;
}
