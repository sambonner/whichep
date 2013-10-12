<?php

/**
 * @class tvdb_api
 * largely a wrapper around curl commands from
 * http://www.thetvdb.com API
 */

class tvdbApi {

  protected $tvdb_domain;
  protected $apikey;

  function __construct($tvdb_domain, $apikey) {
    $this->tvdb_domain = $tvdb_domain;
    $this->apikey = $apikey;
  }

  /**
   * Return series ID for a given series.
   * @param $series_name - the name of the series being requested.
   * @return - the seriesid from TVDB api, assumes the first series return is correct.
   */
  function getSeriesId($series_name) {
    if ($cache_seriesdets = cache_get("whichep:seriesid:$series_name", 'cache_whichep')) {
      $raw_xml = $cache_seriesdets->data;
    }
    else {
      $api_call = $this->tvdb_domain . '/api/GetSeries.php?seriesname=' . $series_name;
      $api_call = curl_init($api_call);
      curl_setopt($api_call, CURLOPT_RETURNTRANSFER, TRUE);
      $raw_xml = curl_exec($api_call);

      if (empty($raw_xml)) {
        return FALSE;
      }

      // Cached series details XML for one week
      $cache_time = time() + 604800;
      cache_set("whichep:seriesid:$series_name", $raw_xml, 'cache_whichep',  $cache_time);
    }

    $series_xml = new domDocument();
    $series_xml->loadXML($raw_xml);

    if (!$series_xml) {
      return FALSE;
    }
    $series_ids = $series_xml->getElementsByTagName('seriesid');



    if (is_object($series_ids->item(0))) {
      return $series_ids->item(0)->nodeValue;
    }
  }

  /**
   * Return XML for series episodes and process.
   * @param $seriesid
   * @return $episodes - an array of domElement objects each containing an episode.
   * Integrated with drupal cache API.
   */
  function getEpList($seriesId) {
    if ($cached_xml = cache_get("whichep:series:$seriesId", 'cache_whichep')) {
      $eplist_raw = $cached_xml->data;
    }

    else {
      // Construct api call
      $api = $this->tvdb_domain . '/api/' . $this->apikey . '/series/' . $seriesId . '/all/en.xml';
      $api_call = curl_init($api);
      curl_setopt($api_call, CURLOPT_RETURNTRANSFER, TRUE);
      $eplist_raw = curl_exec($api_call);

      // Cached series XML for one week
      $cache_time = time() + 604800;
      cache_set("whichep:series:$seriesId", $eplist_raw, 'cache_whichep',  $cache_time);
    }

    // Process XML
    $ep_list_xml = new domDocument();
    $ep_list_xml->loadXML($eplist_raw);
    $eplist = $ep_list_xml->getElementsByTagName('Episode');
    $episodes = array();

    foreach ($eplist as $ep) {
      //Ignore episodes from season 0
      if ($ep->getElementsByTagName('SeasonNumber')->item(0)->nodeValue == '0') {
        unset($ep);
      }
      else {
        $episodes[] = $ep;
      }
    }
    return $episodes;
  }
}
