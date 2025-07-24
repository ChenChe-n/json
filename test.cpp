#include "cc_json.hpp"
#include <iostream>
#include <fstream>

int main()
{
    using js = cc_json;
    cc_json json = js::obj{
        js::kvp{js::key{"id"}, js::arr{js::bol{true}, js::num{2}, js::num{99.57}}},
        js::kvp{js::key{"age"}, js::obj{
                                    {js::key{"张三"}, js::num{18}},
                                    {js::key{"李四"}, js::num{19}},
                                    {js::key{"王五"}, js::num{20.99}}}},
    };
    std::cout << "json--------------------" << std::endl;
    std::cout << json.to_string() << std::endl;

    // 访问数据
    std::cout << "ID[0]: " << (json["id"][0].get_boolean().value_) << std::endl;
    std::cout << "张三的年龄: " << (json["age"]["张三"].get_number().as_string()) << std::endl;
    std::cout << "王五的年龄: " << (json["age"]["王五"].get_number().as_string()) << std::endl;

    // 修改数据
    json["id"][0] = js::num{100};
    json["age"]["李四"] = js::num{20};
    json["city"] = js::str{"Beijing"}; // 添加新字段
    std::cout << "json--------------------" << std::endl;
    std::cout << json.to_string() << std::endl;

    // 访问数据
    std::cout << "ID[0]: " << (json["id"][0].get_number().as_string()) << std::endl;
    std::cout << "张三的年龄: " << (json["age"]["张三"].get_number().as_string()) << std::endl;
    std::cout << "city: " << (json["city"].get_string().value_) << std::endl;

    cc_json json2 = js::obj{
        js::kvp{js::key{"name"}, js::str{"Alice"}},
        js::kvp{js::key{"age"}, js::num{30}},
        js::kvp{js::key{"json1"}, json.data()},
    };
    json2["json1"]["city"] = js::str{"Shanghai"}; // 修改嵌套 JSON 的数据
    json2["json1"]["json1"] = json.data();        //   添加嵌套 JSON
    std::cout << "json--------------------" << std::endl;
    std::cout << json2.to_string() << std::endl;

    cc_json json3 = js::arr{
        js::str{"hello"},
        js::str{"world"},
        js::num{1},
        js::num{9.2222222222222222222222222e141},
        js::num{1.00000001},
        js::num{1.0000000003},
        js::num{"1.0000000003"},
        js::bol{true},
        js::nul{},
        js::obj{
            {"name", js::str{"cc"}},
            {"age", js::num{18}}}};
    std::cout << "json--------------------" << std::endl;
    std::cout << json3.to_string() << std::endl;

    cc_json json4 = cc_json{R"({
  "null_value": null,
  "boolean_true": true,
  "boolean_false": false,
  "integer_number": 123,
  "float_number": 123.456,
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
    "年龄": 25,
    "城市": "北京"
    }})"};
    std::cout << "json--------------------" << std::endl;
    std::cout << json4.to_string() << std::endl;


    // // 读取文件
    // std::string json_file;
    // std::ifstream file("test.json");
    // if (file.is_open()) {
    //     std::string line;
    //     while (std::getline(file, line)) {
    //         json_file += line;
    //     }
    //     file.close();
    // }
    // cc_json json5{json_file};
    // std::cout << "json_file--------------------" << std::endl;
    // std::cout << json5.to_string() << std::endl;

    return 0;
}
