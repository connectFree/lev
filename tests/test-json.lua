--[[

Copyright 2012 The lev Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

--]]

local exports = {}

exports['lev.json:\tencode'] = function(test)
  local lev = require('lev')
  local json = lev.json
  test.ok(json.encode({kristopher="tate"}) == '{"kristopher":"tate"}')
  test.ok(json.encode({1,2,3,4,-12352,5,2934029323094}) == '[1,2,3,4,-12352,5,2934029323094]')
  test.ok(json.encode("I am not a Spaceship") == '"I am not a Spaceship"')
  test.ok(json.encode({1,2,3,4,-12352,5,{warp={1,2,3,4,-12352,5,"2934029323094"}}}) == '[1,2,3,4,-12352,5,{"warp":[1,2,3,4,-12352,5,"2934029323094"]}]')
  test.done()
end


local TWITTER_JSON_EXAMPLE = [[{
  "completed_in":0.031,
  "max_id":122078461840982016,
  "max_id_str":"122078461840982016",
  "next_page":"?page=2&max_id=122078461840982016&q=blue%20angels&rpp=5",
  "page":1,
  "query":"blue+angels",
  "refresh_url":"?since_id=122078461840982016&q=blue%20angels",
  "results":[
    {
      "created_at":"Thu, 06 Oct 2011 19:36:17 +0000",
      "entities":{
        "urls":[
          {
            "url":"http://t.co/L9JXJ2ee",
            "expanded_url":"http://bit.ly/q9fyz9",
            "display_url":"bit.ly/q9fyz9",
            "indices":[
              37,
              57
            ]
          }
        ]
      },
      "from_user":"SFist",
      "from_user_id":14093707,
      "from_user_id_str":"14093707",
      "geo":null,
      "id":122032448266698752,
      "id_str":"122032448266698752",
      "iso_language_code":"en",
      "metadata":{
        "recent_retweets":3,
        "result_type":"popular"
      },
      "profile_image_url":"http://a3.twimg.com/profile_images/51584619/SFist07_normal.jpg",
      "source":"&lt;a href=&quot;http://twitter.com/tweetbutton&quot; rel=&quot;nofollow&quot;&gt;Tweet Button&lt;/a&gt;",
      "text":"Reminder: Blue Angels practice today http://t.co/L9JXJ2ee",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 19:41:12 +0000",
      "entities":{
 
      },
      "from_user":"masters212",
      "from_user_id":2242041,
      "from_user_id_str":"2242041",
      "geo":null,
      "id":122033683212419072,
      "id_str":"122033683212419072",
      "iso_language_code":"en",
      "metadata":{
        "recent_retweets":1,
        "result_type":"popular"
      },
      "profile_image_url":"http://a3.twimg.com/profile_images/488532540/rachel25final_normal.jpg",
      "source":"&lt;a href=&quot;http://twitter.com/&quot;&gt;web&lt;/a&gt;",
      "text":"Starting to hear Blue Angels... Not such angels with all of the noise and carbon pollution.",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 19:39:52 +0000",
      "entities":{
 
      },
      "from_user":"SFBayBridge",
      "from_user_id":182107587,
      "from_user_id_str":"182107587",
      "geo":null,
      "id":122033350327279617,
      "id_str":"122033350327279617",
      "iso_language_code":"en",
      "metadata":{
        "recent_retweets":1,
        "result_type":"popular"
      },
      "profile_image_url":"http://a0.twimg.com/profile_images/1162882917/bbtwitternew_normal.jpg",
      "source":"&lt;a href=&quot;http://twitter.com/&quot;&gt;web&lt;/a&gt;",
      "text":"BZZZzzzZzZzzzZZZZZzZz WHAT? I CAN'T HEAR YOU. THERE ARE BLUE ANGELS. ZZZzzZZZ!",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 22:39:08 +0000",
      "entities":{
 
      },
      "from_user":"OnDST",
      "from_user_id":265656068,
      "from_user_id_str":"265656068",
      "geo":null,
      "id":122078461840982016,
      "id_str":"122078461840982016",
      "iso_language_code":"nl",
      "metadata":{
        "result_type":"recent"
      },
      "profile_image_url":"http://a3.twimg.com/profile_images/1271597598/OnDST_normal.jpg",
      "source":"&lt;a href=&quot;http://dlvr.it&quot; rel=&quot;nofollow&quot;&gt;dlvr.it&lt;/a&gt;",
      "text":"SF Fleet Week to open with Blue Angels flyovers | Student ...",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 22:38:51 +0000",
      "entities":{
 
      },
      "from_user":"gusbumper",
      "from_user_id":15912539,
      "from_user_id_str":"15912539",
      "geo":null,
      "id":122078393641603072,
      "id_str":"122078393641603072",
      "iso_language_code":"en",
      "metadata":{
        "result_type":"recent"
      },
      "profile_image_url":"http://a2.twimg.com/profile_images/832286946/pho_normal.jpg",
      "source":"&lt;a href=&quot;http://itunes.apple.com/us/app/twitter/id409789998?mt=12&quot; rel=&quot;nofollow&quot;&gt;Twitter for Mac&lt;/a&gt;",
      "text":"RT @gzahnd: WAKE UP HIPPIES, THE BLUE ANGELS ARE IN TOWN!",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 22:38:31 +0000",
      "entities":{
 
      },
      "from_user":"LUVTQUILT",
      "from_user_id":32653550,
      "from_user_id_str":"32653550",
      "geo":null,
      "id":122078309004742656,
      "id_str":"122078309004742656",
      "iso_language_code":"en",
      "metadata":{
        "result_type":"recent"
      },
      "profile_image_url":"http://a1.twimg.com/profile_images/1188428056/IMG00007-20100521-1647_1__normal.jpg",
      "source":"&lt;a href=&quot;http://ubersocial.com&quot; rel=&quot;nofollow&quot;&gt;\u00DCberSocial for BlackBerry&lt;/a&gt;",
      "text":"Thursday - Just watched the Blue Angels practice over SF Bay Impressive! What a background.  GGB & Alcatraz. ;) .",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 22:38:22 +0000",
      "entities":{
        "urls":[
          {
            "url":"http://t.co/fyL8Rs5f",
            "expanded_url":"http://dlvr.it/pfFfj",
            "display_url":"dlvr.it/pfFfj",
            "indices":[
              52,
              72
            ]
          }
        ]
      },
      "from_user":"johnnyfuncheap",
      "from_user_id":20717004,
      "from_user_id_str":"20717004",
      "geo":null,
      "id":122078271478317056,
      "id_str":"122078271478317056",
      "iso_language_code":"en",
      "metadata":{
        "result_type":"recent"
      },
      "profile_image_url":"http://a0.twimg.com/profile_images/1130541908/funcheap_icon_twitter_normal.gif",
      "source":"&lt;a href=&quot;http://dlvr.it&quot; rel=&quot;nofollow&quot;&gt;dlvr.it&lt;/a&gt;",
      "text":"10/8/11: Blue Angels Wine Tasting | Treasure Island http://t.co/fyL8Rs5f",
      "to_user_id":null,
      "to_user_id_str":null
    },
    {
      "created_at":"Thu, 06 Oct 2011 22:37:28 +0000",
      "entities":{
        "urls":[
          {
            "url":"http://t.co/KfzEqOWM",
            "expanded_url":"http://married2travel.com/2600/san-francisco-day3-golden-gate-park-pier-39-blue-angels/",
            "display_url":"married2travel.com/2600/san-franc\u2026",
            "indices":[
              47,
              67
            ]
          }
        ]
      },
      "from_user":"espenorio",
      "from_user_id":52736683,
      "from_user_id_str":"52736683",
      "geo":null,
      "id":122078043664695296,
      "id_str":"122078043664695296",
      "iso_language_code":"en",
      "metadata":{
        "result_type":"recent"
      },
      "profile_image_url":"http://a0.twimg.com/profile_images/1574863913/sheil_normal.png",
      "source":"&lt;a href=&quot;http://twitter.com/&quot;&gt;web&lt;/a&gt;",
      "text":"San Francisco 2010 Fleet week photos and video http://t.co/KfzEqOWM",
      "to_user_id":null,
      "to_user_id_str":null
    }
  ],
  "results_per_page":5,
  "since_id":0,
  "since_id_str":"0"
}]]

exports['lev.json:\tdecode'] = function(test)
  local lev = require('lev')
  local json = lev.json
  local decoded = json.decode( TWITTER_JSON_EXAMPLE )
  test.ok( decoded )
  if decoded then
    test.ok( decoded.page == 1 )
    test.ok( decoded.refresh_url == "?since_id=122078461840982016&q=blue%20angels" )
    test.ok( decoded.completed_in == 0.031 )
    test.ok( decoded.results[3] )
    test.ok( decoded.results[3].metadata )
    test.ok( decoded.results[3].metadata.recent_retweets == 1 )
    test.ok( decoded.results[3].to_user_id == json.null )
  end
  test.done()
end

exports['lev.json:\tnull'] = function(test)
  local lev = require('lev')
  local json = lev.json
  test.ok(json.encode({this_is_null=json.null}) == '{"this_is_null":null}')
  local decoded = json.decode('{"this_is_null":null}')
  test.ok( decoded )
  if decoded then
    test.ok( decoded.this_is_null == json.null )
  end
  test.done()
end

return exports
