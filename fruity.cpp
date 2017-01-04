#include <yaml-cpp/yaml.h>
#include <iostream>
#include <sstream>
#include <twitter.h>
#include <verbly.h>
#include <chrono>
#include <thread>
#include <random>

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << "usage: fruity [configfile]" << std::endl;
    return -1;
  }

  std::random_device random_device;
  std::mt19937 random_engine{random_device()};

  std::string configfile(argv[1]);
  YAML::Node config = YAML::LoadFile(configfile);

  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());

  twitter::client client(auth);

  verbly::data database {config["verbly_datafile"].as<std::string>()};

  verbly::noun fruit1 = database.nouns().with_wnid(113134947).run().front(); // fruit
  verbly::noun fruit2 = database.nouns().with_wnid(107705931).run().front(); // edible fruit
  verbly::filter<verbly::noun> fruitFilter {fruit1, fruit2};
  fruitFilter.set_orlogic(true);

  verbly::noun plants = database.nouns().with_wnid(100017222).run().front(); // plant
  verbly::noun drugs = database.nouns().with_wnid(103247620).run().front(); // drug
  verbly::noun animals = database.nouns().with_wnid(100015388).run().front(); // animal

  for (;;)
  {
    std::cout << "Generating tweet" << std::endl;

    auto n1 = database.nouns().full_hyponym_of(fruitFilter).random().limit(1).run();
    auto n1w = n1.front();
    auto n1hq = database.nouns().hypernym_of(n1w).limit(1).run();
    auto n1h = n1hq.front();

    std::list<std::string> tokens;

    int choice = std::uniform_int_distribution<int>(0,2)(random_engine);
    if (choice == 0)
    {
      auto descriptor = database.adjectives().is_pertainymic().random().limit(1).run();
      tokens.push_back(descriptor.front().base_form());
    } else if (choice == 1)
    {
      auto descriptor = database.adjectives().is_mannernymic().random().limit(1).run();
      tokens.push_back(descriptor.front().base_form());
    }

    auto plantThing = database.nouns();
    choice = std::uniform_int_distribution<int>(0,4)(random_engine);
    if (choice < 3)
    {
      if (choice == 0)
      {
        plantThing.full_hyponym_of(plants);
      } else if (choice == 1)
      {
        plantThing.full_hyponym_of(drugs);
      } else if (choice == 2)
      {
        plantThing.full_hyponym_of(animals);
      }

      plantThing.is_not_proper().with_complexity(1).random().limit(1);
      tokens.push_back(plantThing.run().front().base_form());
    }

    auto similar = database.nouns().full_hyponym_of(n1h).except(n1w).is_not_proper().with_complexity(1).random().limit(1).run();
    if (!similar.empty())
    {
      tokens.push_back(similar.front().base_form());
    } else {
      auto different = database.nouns().full_hyponym_of(fruitFilter).except(n1w).is_not_proper().with_complexity(1).random().limit(1).run();
      tokens.push_back(different.front().base_form());
    }

    std::string fruitName = verbly::implode(std::begin(tokens), std::end(tokens), " ");

    std::ostringstream result;
    result << n1.front().base_form();
    result << "? ";

    choice = std::uniform_int_distribution<int>(0,3)(random_engine);
    if (choice == 0)
    {
      result << "Oh, you mean ";
      result << fruitName;
    } else if (choice == 1)
    {
      result << "Don't you mean ";
      result << fruitName;
      result << "?";
    } else if (choice == 2)
    {
      result << "You mean ";
      result << fruitName;
      result << "!";
    } else if (choice == 3)
    {
      result << "Pfft, everyone just calls it ";
      result << fruitName;
      result << " now";
    }

    std::string tweet = result.str();
    tweet.resize(140);

    try
    {
      client.updateStatus(tweet);
      
      std::cout << tweet << std::endl;
    } catch (const twitter::twitter_error& e)
    {
      std::cout << "Twitter error: " << e.what() << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::hours(1));
  }
}
