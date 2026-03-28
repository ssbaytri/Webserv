<?php
// Set content type
header("Content-Type: text/plain");

// Output received POST data
echo "Received POST data:\n";
echo file_get_contents("php://input") . "\n";

// Output some CGI environment variables
echo "REQUEST_METHOD = " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "QUERY_STRING = " . ($_SERVER['QUERY_STRING'] ?? '') . "\n";
?>