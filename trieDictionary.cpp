#include <gtkmm.h> // Include the necessary gtkmm headers
#include <cctype>
#include <fstream>
#include <string>
#include <iostream>

using namespace std;
const int ALPHA_SIZE = 37;

class Trie {
public:

	Trie* childNode[ALPHA_SIZE];
	string prefix;
	bool isValidWord;
	
	// Constructor
	Trie(string pre = "") : prefix(pre), isValidWord(false) {
		for(int i = 0; i < ALPHA_SIZE; i++) {
			childNode[i] = nullptr;
		}
	}
	
	Trie(const Trie& node) : prefix(node.prefix), isValidWord(node.isValidWord){
		for(int i = 0; i < ALPHA_SIZE; i++) {
			if (node.childNode[i] != nullptr) {
				childNode[i] = new Trie(*node.childNode[i]);
			} else {
				childNode[i] = nullptr;
			}
		}
	};
	
	~Trie(){
		for(int i = 0; i < ALPHA_SIZE; i++){
			if (childNode[i] != nullptr) {
				delete childNode[i];
			}
		}
	}
};

int charToIndex(char c){
	if (isdigit(c)){
		return c - '0';
	} else if(islower(c)){
		return c - 'a' + 10;
	} else if(c == '_' || c == ' '){
		return 36;
	} else {
		return -1;
	}
}

//Insert
void insert_word(Trie* root, const string& word){
	
	Trie* currentNode = root;
	for(int i = 0; i < word.size(); i++){
		int index = charToIndex(word[i]);
		if(index != -1){
			if(currentNode -> childNode[index] == nullptr){
				string newPrefix = currentNode->prefix + word[i];
				currentNode->childNode[index] = new Trie(newPrefix);
			}
			currentNode = currentNode->childNode[index];
		} else if(!currentNode->prefix.empty()){
			cout << "Invalid entry: " << currentNode->prefix << endl;
			return;
		}
	}
	
	currentNode->isValidWord = true;
}
//Auto complete
void auto_complete(Trie* currentNode, vector<string>& suggestions, int& valid_option){
	string auto_results;
	if(valid_option >= 3) {
		return;
	}
	
	if(currentNode->isValidWord) {
		suggestions.push_back(currentNode->prefix);
		valid_option += 1;
		if(valid_option >= 3) return;
	}
	
	for(int i = 0; i < ALPHA_SIZE && valid_option < 3; ++i){
		if(currentNode->childNode[i] != nullptr){
			auto_complete(currentNode->childNode[i], suggestions, valid_option);
		}
	}
}

// Search
vector<string> search(Trie* root, const string& word){
	Trie* currentNode = root;
	Trie* validNode = currentNode;
	vector<string> results;
	string lower_word;
	for(char c : word) {
		lower_word += tolower(c);
	}
	int valid_option = 0;
	for(int i = 0; i < lower_word.size(); i++){
		int index = charToIndex(lower_word[i]);
		if(index == -1 || index >= ALPHA_SIZE || currentNode->childNode[index] == nullptr){
			results.push_back(word + " not found:");
			results.push_back("Did you mean:");
			auto_complete(currentNode, results, valid_option);
			return results;
		}
		currentNode = currentNode->childNode[index];
		validNode = currentNode;
	}
	if(currentNode != nullptr && currentNode->isValidWord){
		 results.push_back(word + " was found");
	}
	if(currentNode != nullptr){
		results.push_back("Auto Complete: ");
		auto_complete(currentNode, results, valid_option);
	}
	return results;
}

class MyWindow : public Gtk::Window {
public:
    MyWindow()
    {
        // Set up the button
		m_button.set_label("Search");
		m_button.set_size_request(100, -1);
		
		// Set up entry
		m_entry.set_size_request(200, -1);
		m_combo.set_size_request(200, -1);
		
		
		m_button.signal_clicked().connect(sigc::mem_fun(*this, &MyWindow::on_button_clicked));
		m_entry.signal_changed().connect(sigc::mem_fun(*this, &MyWindow::on_combo_changed));
		m_entry.signal_key_press_event().connect(sigc::mem_fun(*this, &MyWindow::on_entry_key_press_event), false);
			
		// Organize widgets in the box
		m_box.pack_start(m_entry, Gtk::PACK_SHRINK, 10); // Add the entry widget to the box
		m_box.pack_start(m_combo, Gtk::PACK_SHRINK, 10);
		m_box.pack_start(m_button); // Add the button widget to the box
		m_box.pack_start(m_search);
		
		m_box.set_spacing(5);
		 // Add the button to the window
		add(m_box);
		
		// Show all children of the window
		show_all_children();
		
		loadDictionary("./dictionary.txt");
	}

protected:
	void on_button_clicked() {
		string searchText = m_entry.get_text(); // Get text from a Gtk::Entry widget
		auto results = search(&m_trie, searchText); // Assume 'trie' is your Trie instance

		// Convert results to a single string or format as needed for display
		string displayText;
		for (const auto& result : results) {
			displayText += result + "\n"; // Simple newline-separated list
		}

		// Update a Gtk::TextView or other widget with the search results
		m_search.get_buffer()->set_text(displayText);
	}
	
	bool on_entry_key_press_event(GdkEventKey* key_event) {
		if (key_event->keyval == GDK_KEY_Return || key_event->keyval == GDK_KEY_KP_Enter) {
			on_button_clicked(); // Perform the search action
			return true; // Stops further processing, indicating the event has been handled
		}
		return false; // Allows other handlers, including the default text input handling
	}
	
	void loadDictionary(const string& dictPath) {
		ifstream file(dictPath);
		if (!file) {
			cerr << "Could not open the file: " << dictPath << endl;
			return; // Optionally, handle the error more gracefully
		}

		string currentWord;
		
		char ch;
		while (file.get(ch)){
			if(ch == '\n') {
				if (!currentWord.empty()){
					insert_word(&m_trie, currentWord);
					currentWord.clear();
				}
			} else if (ch != ' '){
				currentWord += ch;
			}
		}

		if (!currentWord.empty()){
			insert_word(&m_trie, currentWord);
		}

		file.close(); // Ensure the file is closed after reading
	}
	
	Trie* find_node_for_partial_word(Trie* root, const string& partial) {
		Trie* currentNode = root;
		for (char c : partial) {
			int index = charToIndex(c);
			if (index == -1 || !currentNode->childNode[index]) {
				return nullptr; // Partial word not found
			}
			currentNode = currentNode->childNode[index];
		}
		return currentNode; // Node corresponding to the end of the partial word
	}
	
	void on_combo_changed() {
	    // This should be connected to the text entry's signal_changed, not the combo box's signal_changed
	    auto entry = m_combo.get_entry();
	    if (entry) {
	        string currentText = entry->get_text();
	        // Convert currentText to lowercase for comparison, if needed
	        string lowerText;
	        for(char c : currentText) {
	            lowerText += tolower(c);
	        }
        
	        vector<string> suggestions;
	        if(!lowerText.empty()){
	            Trie* partialNode = find_node_for_partial_word(&m_trie, lowerText);
	            if (partialNode) {
	                int valid_option = 0; // Use this to limit the number of suggestions if desired
	                auto_complete(partialNode, suggestions, valid_option);
	                // You might not need autoWord separately unless for different handling
	            }
	        }
        
	        update_combo_suggestions(suggestions);
	    }
	}
	
	void update_suggestions_display(const vector<string>& suggestions) {
		string displayText;
		for (const auto& suggestion : suggestions) {
			displayText += suggestion + "\n";
		}

		m_search.get_buffer()->set_text(displayText);
	}
	
	void update_combo_suggestions(const vector<string>& suggestions) {
	    m_combo.remove_all(); // Clear existing items
	    for (const auto& suggestion : suggestions) {
	        m_combo.append(suggestion); // Add new suggestions
	    }
	    if (!suggestions.empty()) {
	        m_combo.popup(); // Optionally show the suggestions dropdown
	    }
	}
	
	// void update_search_display(const string& currentText, const vector<string>& suggestions) {
// 	    Glib::RefPtr<Gtk::TextBuffer> buffer = m_combo.get_buffer();
//
// 	    // Check if the tag already exists, if not create it
// 	    Glib::RefPtr<Gtk::TextTagTable> tagTable = buffer->get_tag_table();
// 	    Glib::RefPtr<Gtk::TextTag> tag_red;
// 	    if (!tagTable->lookup("red_text")) {
// 	        tag_red = buffer->create_tag("red_text");
// 	        tag_red->property_foreground() = "red";
// 	    } else {
// 	        tag_red = tagTable->lookup("red_text");
// 	    }
//
// 	    // Clear the buffer and set the user's current input text
// 	    buffer->set_text(currentText);
//
// 	    // Convert currentText to lowercase for comparison
// 	    string currentTextLower = currentText;
//
// 	    // Append suggestions, applying the red text tag only to suggestions
// 	    for (const auto& suggestion : suggestions) {
// 	        string suggestionLower = suggestion;
//
// 	        // If suggestion starts with the current text, append it with the red tag
// 	        if (suggestionLower.substr(0, currentTextLower.length()) == currentTextLower) {
// 	            // Remove the matching part from the beginning of the suggestion
// 	            string trimmedSuggestion = suggestion.substr(currentTextLower.length());
// 	            // Move the iterator to the end of the buffer, insert the suggestion, and apply the red tag
// 	            Gtk::TextBuffer::iterator iter = buffer->end();
// 	            buffer->insert(iter, trimmedSuggestion);
// 	            iter = buffer->end(); // Update iterator since text was inserted
// 	            iter.backward_chars(trimmedSuggestion.length()); // Move back to the start of the inserted text
// 	            buffer->apply_tag(tag_red, iter, buffer->end()); // Apply red tag to the inserted suggestion
// 	        }
// 	    }
// 	}
	
	
	
private:
	Trie m_trie; // Trie member variable
	Gtk::Box m_box{Gtk::ORIENTATION_VERTICAL}; // Box to organize widgets
	Gtk::Button m_button; // Search button
	Gtk::Entry m_entry; // Text entry for search queries
	Gtk::TextView m_search; // TextView to display search results or suggestions
	Gtk::ComboBoxText m_combo{true};
};


int main(int argc, char *argv[])
{
	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

	MyWindow window;
	window.set_default_size(400, 200);
	
	return app->run(window);
}