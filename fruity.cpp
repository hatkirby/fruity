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

  verbly::database database(config["verbly_datafile"].as<std::string>());

  verbly::filter fruitFilter = (
    (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
    && (verbly::notion::fullHypernyms %= (
      (verbly::notion::wnid == 113134947) // fruit
      || (verbly::notion::wnid == 107705931)))); // edible fruit

  verbly::query<verbly::word> fruitQuery = database.words(fruitFilter);

  verbly::query<verbly::word> pertainymQuery = database.words(
    (verbly::notion::partOfSpeech == verbly::part_of_speech::adjective)
    && (verbly::word::antiPertainyms));

  verbly::query<verbly::word> antiMannernymQuery = database.words(
    (verbly::notion::partOfSpeech == verbly::part_of_speech::adjective)
    && (verbly::word::mannernyms));

  for (;;)
  {
    std::cout << "Generating tweet" << std::endl;

    verbly::word fruit = fruitQuery.first();
    verbly::word hyper = database.words(verbly::notion::hyponyms %= fruit).first();

    verbly::token utterance;

    int choice = std::uniform_int_distribution<int>(0,2)(random_engine);
    if (choice == 0)
    {
      utterance << pertainymQuery.first();
    } else if (choice == 1)
    {
      utterance << antiMannernymQuery.first();
    }

    verbly::filter thingFilter = (
      (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
      && (verbly::form::proper == false)
      && (verbly::form::complexity == 1));

    choice = std::uniform_int_distribution<int>(0,4)(random_engine);
    if (choice < 3)
    {
      if (choice == 0)
      {
        // plant
        thingFilter &= (verbly::notion::fullHypernyms %= (verbly::notion::wnid == 100017222));
      } else if (choice == 1)
      {
        // drug
        thingFilter &= (verbly::notion::fullHypernyms %= (verbly::notion::wnid == 103247620));
      } else if (choice == 2)
      {
        // animal
        thingFilter &= (verbly::notion::fullHypernyms %= (verbly::notion::wnid == 100015388));
      }

      utterance << database.words(thingFilter).first();
    }

    verbly::query<verbly::word> similarQuery = database.words(
      (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
      && (verbly::notion::fullHypernyms %= hyper)
      && !fruit.getBaseForm()
      && !hyper
      && (verbly::form::proper == false)
      && (verbly::form::complexity == 1));
    std::vector<verbly::word> similarResults = similarQuery.all();

    if (!similarResults.empty())
    {
      utterance << similarResults.front();
    } else {
      verbly::query<verbly::word> differentQuery = database.words(
        fruitFilter
        && !fruit.getBaseForm()
        && !hyper
        && (verbly::form::proper == false)
        && (verbly::form::complexity == 1));

      utterance << differentQuery.first();
    }

    std::string fruitName = utterance.compile();

    std::ostringstream result;
    result << fruit.getBaseForm().getText();
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
