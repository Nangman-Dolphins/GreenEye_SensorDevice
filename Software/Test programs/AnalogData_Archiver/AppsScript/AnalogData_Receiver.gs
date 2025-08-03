/*
 * Handles GET requests. This function is a wrapper for handleData.
 * @param {Object} e The event parameter from the GET request.
 * @return {ContentService.TextOutput} The JSON response.
 */
function doGet(e) {
  return handleData(e);
}

/*
 * Handles POST requests. This function is a wrapper for handleData.
 * @param {Object} e The event parameter from the POST request.
 * @return {ContentService.TextOutput} The JSON response.
 */
function doPost(e) {
  return handleData(e);
}

/*
 * The main function to process incoming data and write it to the spreadsheet.
 * @param {Object} e The event parameter from the request.
 * @return {ContentService.TextOutput} The JSON response indicating success or failure.
 */
function handleData(e) {
  const lock = LockService.getScriptLock();
  lock.waitLock(10000); // Wait a maximum of 10 seconds for other processes to finish.

  try {
    // Get the sheet name from the URL parameter 'sheet'. If not provided, default to "Sheet1".
    const sheetName = e.parameter.sheet || "Sheet1";
    
    const doc = SpreadsheetApp.getActiveSpreadsheet();
    let sheet = doc.getSheetByName(sheetName);

    // If a sheet with the specified name doesn't exist, create it.
    if (!sheet) {
      sheet = doc.insertSheet(sheetName);
      // Add headers to the new sheet. You can customize these.
      const headers = ["Timestamp", "Sensor_Type", "Value1", "Value2"]; 
      sheet.getRange(1, 1, 1, headers.length).setValues([headers]);
    }
    
    // Get the headers from the first row of the sheet.
    const headers = sheet.getRange(1, 1, 1, sheet.getLastColumn()).getValues()[0];
    const nextRow = sheet.getLastRow() + 1; // Get the next available row.

    // Create a new row of data based on the headers and URL parameters.
    const newRow = headers.map(function(header) {
      // If the header is "Timestamp", add the current date. Otherwise, use the parameter value.
      return header === "Timestamp" ? new Date() : e.parameter[header];
    });

    // Set the values in the new row.
    sheet.getRange(nextRow, 1, 1, newRow.length).setValues([newRow]);

    // Return a success message as a JSON object.
    return ContentService
      .createTextOutput(JSON.stringify({ "result": "success", "row": nextRow, "sheet": sheetName }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  catch (err) {
    // Return an error message if something goes wrong.
    return ContentService
      .createTextOutput(JSON.stringify({ "result": "error", "error": err }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  finally {
    // Release the lock to allow other processes to run.
    lock.releaseLock();
  }
}